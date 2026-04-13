#include <atomic>
#include <chrono>
#include <iostream>
#include <pipewire/pipewire.h>
#include <spa/param/audio/format-utils.h>
#include <string>
#include <thread>
#include <vector>

#include "ModulationStrategies/modulationStrategy.h"
#include "RingBuffer.h"
#include "lib/filter.h"
#include "lib/modulation.h"
#include "lib/waveforms.h"
#include "pipewire/main-loop.h"
#include "pipewire/properties.h"
#include "pipewire/stream.h"
#include "tests/transferTest.h"

// --- Global/Context Structure ---
struct Context {
  struct pw_main_loop *loop;
  struct pw_stream *capture_stream;
  struct pw_stream *playback_stream;

  Ringbuffer<float> *input_buffer;
  Ringbuffer<float> *output_buffer;

  ProtocolConfig p;
  SignalModulation *s;
  std::atomic<bool> running;
};

// --- PipeWire Callbacks ---

static void on_process_playback(void *userdata) {
  Context *app = (Context *)userdata;
  struct pw_buffer *b;
  if (!(b = pw_stream_dequeue_buffer(app->playback_stream)))
    return;

  struct spa_buffer *buf = b->buffer;
  float *dst = (float *)buf->datas[0].data;
  uint32_t n_frames = buf->datas[0].maxsize / sizeof(float);

  for (uint32_t i = 0; i < n_frames; i++) {
    float sample = 0.0f;
    if (app->output_buffer && app->output_buffer->size() > 0) {
      app->output_buffer->pop(sample);
    }
    dst[i] = sample;
  }

  buf->datas[0].chunk->offset = 0;
  buf->datas[0].chunk->size = n_frames * sizeof(float);
  buf->datas[0].chunk->stride = sizeof(float);
  pw_stream_queue_buffer(app->playback_stream, b);
}

static void on_process_capture(void *userdata) {
  Context *app = (Context *)userdata;
  struct pw_buffer *b;
  if (!(b = pw_stream_dequeue_buffer(app->capture_stream)))
    return;
  if (app->input_buffer == nullptr)
    return;

  struct spa_buffer *buf = b->buffer;
  float *src = (float *)buf->datas[0].data;
  if (src) {
    uint32_t n_frames = buf->datas[0].chunk->size / sizeof(float);
    for (uint32_t i = 0; i < n_frames; i++) {
      app->input_buffer->add(src[i]);
    }
  }
  pw_stream_queue_buffer(app->capture_stream, b);
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
  app.running = true;

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

void analyzeFrequencies(Context &app) {
  std::thread worker([&]() {
    std::vector<float> frame(app.p.N);
    while (app.running) {
      if (app.input_buffer->size() >= app.p.N) {
        for (int i = 0; i < app.p.N; i++)
          app.input_buffer->pop(frame[i]);
        app.s->enqueue_frame(frame);
      } else {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
      }
    }
  });

  pw_main_loop_run(app.loop);
  app.running = false;
  worker.join();
}

void sendData(Context &app, std::string data) {
  std::vector<uint8_t> vec(data.begin(), data.end());
  app.s->transmit_data(vec);

  // Watcher thread to close when done
  std::thread watcher([&]() {
    while (app.output_buffer->size() > 0) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    pw_main_loop_quit(app.loop);
  });

  pw_main_loop_run(app.loop);
  watcher.join();
}

void run_tx_tests(Context* app) {
  std::thread runner([&]() {
    test_tx(app->s, [&](waveform w) {
      app->output_buffer->resize(w.size());
      for(float s: w) {
        app->output_buffer->add(s);
      }

      while (app->output_buffer->size() > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    });
    pw_main_loop_quit(app->loop);
  });

  pw_main_loop_run(app->loop);
  runner.join();
  pw_main_loop_quit(app->loop);
}

int main(int argc, const char *argv[]) {
  if (argc < 2) {
    std::cout << "Usage: " << argv[0] << " [detect|play|send] [data]"
              << std::endl;
    return 1;
  }

  Context app;
  app.p.N = 1024;
  app.p.sample_rate = 48000;
  app.input_buffer = new Ringbuffer<float>(30 * app.p.N);
  app.output_buffer = new Ringbuffer<float>(10 * app.p.sample_rate);

  app.p = *createProtocolConfig(1024);
  app.p.lowest_strength = -100;
  app.p.strength_threshold = -45;

  app.s = new SignalModulation(app.p);
  std::vector<int> freqs = {app.p.f1, app.p.f1 + 10, app.p.f2, app.p.f2 + 10};
  app.s->set_strategy(new TwoTonePerBitModulationStrategy(freqs));
  app.s->set_tx_callback([&](waveform w) {
    app.output_buffer->resize(w.size());
    for (float a : w)
      app.output_buffer->add(a);
  });

  init_pw_common(app);

  std::string cmd(argv[1]);
  if (cmd == "detect") {
    analyzeFrequencies(app);
  } else if (cmd == "send" || cmd == "play") {
    sendData(app, argv[2]);
  } else if (cmd == "tx-tests") {
    run_tx_tests(&app);
  }

  return 0;
}
