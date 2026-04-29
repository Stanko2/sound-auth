#include <iostream>
#include <pipewire/pipewire.h>
#include <spa/param/audio/format-utils.h>
#include <string>
#include <thread>
#include <vector>

#include "ModulationStrategies/modulationStrategy.h"
#include "RingBuffer.h"
#include "lib/entry.h"
#include "lib/modulation.h"
#include "pipewire/main-loop.h"
#include "pipewire/properties.h"
#include "pipewire/stream.h"
#include "tests/transferTest.h"

// --- Global/Context Structure ---
struct Context {
  struct pw_main_loop *loop;
  struct pw_stream *capture_stream;
  struct pw_stream *playback_stream;

  SoundTransfer* t;
};

// --- PipeWire Callbacks ---

static void on_process_playback(void *userdata) {
  Context *ctx = (Context *)userdata;
  struct pw_buffer *b;
  if (!(b = pw_stream_dequeue_buffer(ctx->playback_stream)))
    return;

  struct spa_buffer *buf = b->buffer;
  float *dst = (float *)buf->datas[0].data;
  uint32_t n_frames = buf->datas[0].maxsize / sizeof(float);

  for (uint32_t i = 0; i < n_frames; i++) {
    float sample = 0.0f;
    Ringbuffer<float>* output_buffer = ctx->t->get_output_buffer();
    if (output_buffer && output_buffer->size() > 0) {
      output_buffer->pop(sample);
    }
    dst[i] = sample;
  }

  buf->datas[0].chunk->offset = 0;
  buf->datas[0].chunk->size = n_frames * sizeof(float);
  buf->datas[0].chunk->stride = sizeof(float);
  pw_stream_queue_buffer(ctx->playback_stream, b);
}

static void on_process_capture(void *userdata) {
  Context *ctx = (Context *)userdata;
  struct pw_buffer *b;
  if (!(b = pw_stream_dequeue_buffer(ctx->capture_stream)))
    return;
  Ringbuffer<float>* input_buffer = ctx->t->get_input_buffer();
  if (input_buffer == nullptr)
    return;

  struct spa_buffer *buf = b->buffer;
  float *src = (float *)buf->datas[0].data;
  if (src) {
    uint32_t n_frames = buf->datas[0].chunk->size / sizeof(float);
    for (uint32_t i = 0; i < n_frames; i++) {
      input_buffer->add(src[i]);
    }
  }
  pw_stream_queue_buffer(ctx->capture_stream, b);
}

static void on_stream_state_changed(void *data, enum pw_stream_state old,
                                    enum pw_stream_state state,
                                    const char *error) {
  printf("Stream state changed: %s -> %s\n", pw_stream_state_as_string(old),
         pw_stream_state_as_string(state));
  if (state == PW_STREAM_STATE_ERROR) {
    fprintf(stderr, "Stream error: %s\n", error);
  }
}

static const struct pw_stream_events capture_events = {
    .version = PW_VERSION_STREAM_EVENTS,
    .state_changed = on_stream_state_changed,
    .process = on_process_capture};
static const struct pw_stream_events playback_events = {
    .version = PW_VERSION_STREAM_EVENTS,
    .state_changed = on_stream_state_changed,
    .process = on_process_playback};

void init_pw_common(Context &app) {
  pw_init(NULL, NULL);
  app.loop = pw_main_loop_new(NULL);

  struct spa_audio_info_raw info =
      SPA_AUDIO_INFO_RAW_INIT(.format = SPA_AUDIO_FORMAT_F32, .rate = 48000,
                              .channels = 1);

  uint8_t buffer[1024];
  struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));
  const struct spa_pod *params[1];
  params[0] = spa_format_audio_raw_build(&b, SPA_PARAM_EnumFormat, &info);

  struct pw_properties *capture_props = pw_properties_new(
      PW_KEY_MEDIA_TYPE, "Audio", PW_KEY_MEDIA_CATEGORY, "Capture",
      PW_KEY_MEDIA_ROLE, "Communication", PW_KEY_NODE_NAME,
      "sound-transfer-capture", PW_KEY_NODE_DESCRIPTION,
      "Frequency Detector Input", "media.class", "Stream/Input/Audio", NULL);

  struct pw_properties *playback_props = pw_properties_new(
      PW_KEY_MEDIA_TYPE, "Audio", PW_KEY_MEDIA_CATEGORY, "Playback",
      PW_KEY_MEDIA_ROLE, "Communication", PW_KEY_NODE_NAME,
      "sound-transfer-playback", PW_KEY_NODE_DESCRIPTION,
      "Data Transmitter Output", "media.class", "Stream/Output/Audio", NULL);

  pw_properties_set(capture_props, "target.object", "@DEFAULT_SOURCE@");
  pw_properties_set(playback_props, "target.object", "@DEFAULT_SINK@");

  app.capture_stream =
      pw_stream_new_simple(pw_main_loop_get_loop(app.loop), "Capture",
                           capture_props, &capture_events, &app);

  app.playback_stream =
      pw_stream_new_simple(pw_main_loop_get_loop(app.loop), "Playback",
                           playback_props, &playback_events, &app);

  pw_stream_connect(app.capture_stream, PW_DIRECTION_INPUT, PW_ID_ANY,
                    (pw_stream_flags)(PW_STREAM_FLAG_AUTOCONNECT |
                                      PW_STREAM_FLAG_MAP_BUFFERS),
                    params, 1);

  pw_stream_connect(app.playback_stream, PW_DIRECTION_OUTPUT, PW_ID_ANY,
                    (pw_stream_flags)(PW_STREAM_FLAG_AUTOCONNECT |
                                      PW_STREAM_FLAG_RT_PROCESS |
                                      PW_STREAM_FLAG_MAP_BUFFERS),
                    params, 1);
}

void sendData(Context &app, std::string data) {
  std::vector<uint8_t> vec(data.begin(), data.end());
  app.t->send(vec);
}

int main(int argc, const char *argv[]) {
  if (argc < 2) {
    std::cout << "Usage: " << argv[0] << " [detect|play|send] [data]"
              << std::endl;
    return 1;
  }

  Context ctx;
  ProtocolConfig p;
  p = *createProtocolConfig(1024, 48000, 6000, 8000);
  p.lowest_strength = -100;
  p.strength_threshold = -60;

  ModulationStrategy* strategy = new TwoTonePerBitModulationStrategy(p.f1, 4, 5);

  std::cout << strategy << std::endl;
  ctx.t = new SoundTransfer(strategy, &p);

  init_pw_common(ctx);

  // std::thread loop_thread = run_main_loop(&ctx);
  std::thread audio_thread([&ctx](){
    std::cout << "audio-thread started" << std::endl;
    pw_main_loop_run(ctx.loop);
  });
  ctx.t->run();

  std::string cmd(argv[1]);
  if (cmd == "detect") {
    // analyzeFrequencies(ctx);
    ctx.t->recv();
  } else if (cmd == "send" || cmd == "play") {
    sendData(ctx, argv[2]);
  } else if (cmd == "tx-tests") {
    test_tx(ctx.t, 8, 32);
  } else if (cmd == "rx-tests") {
    test_rx(ctx.t, 8, 32);
  }


  // loop_thread.join();
  std::cout << "quit" << std::endl;
  // audio_thread.join();
  pw_main_loop_quit(ctx.loop);

  return 0;
}
