#include "audio-control.h"
#include "../config.h"
#include "../sound-transfer-lib/RingBuffer.h"
#include "pipewire/context.h"
#include "pipewire/core.h"
#include "pipewire/keys.h"
#include "pipewire/loop.h"
#include "pipewire/main-loop.h"
#include "pipewire/pipewire.h"
#include "pipewire/port.h"
#include "pipewire/properties.h"
#include "pipewire/stream.h"
#include "spa/param/audio/raw-utils.h"
#include "spa/param/audio/raw.h"
#include "spa/param/param.h"
#include "spa/pod/builder.h"
#include <chrono>
#include <cstdint>
#include <iostream>
#include <thread>

GGWave::SampleFormat AudioControl::getInputSampleFormat() {

  return GGWAVE_SAMPLE_FORMAT_F32;
}

GGWave::SampleFormat AudioControl::getOutputSampleFormat() {
  return GGWAVE_SAMPLE_FORMAT_F32;
}

int AudioControl::getInputSampleRate() { return 48000; }

int AudioControl::getOutputSampleRate() { return 48000; }

void AudioControl::listAllDevices() {
  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  std::cout << "[Playback devices]" << std::endl;
  for (auto &x : playback_devices) {
    std::cout << " - " << x.first << std::endl;
  }

  std::cout << "[Capture devices]" << std::endl;
  for (auto &x : capture_devices) {
    std::cout << " - " << x.first << std::endl;
  }
}

AudioControl::AudioControl() {
  pw_init(NULL, NULL);
  main_loop = pw_main_loop_new(nullptr);

  context = pw_context_new(pw_main_loop_get_loop(main_loop), nullptr, 0);

  core = pw_context_connect(context, nullptr, 0);

  registry = pw_core_get_registry(core, PW_VERSION_REGISTRY, 0);

  struct spa_audio_info_raw info =
      SPA_AUDIO_INFO_RAW_INIT(.format = SPA_AUDIO_FORMAT_F32, .rate = 48000,
                              .channels = 1);

  struct spa_pod_builder b =
      SPA_POD_BUILDER_INIT(audio_buffer, sizeof(audio_buffer));
  params[0] = spa_format_audio_raw_build(&b, SPA_PARAM_EnumFormat, &info);

  if (!params[0]) {
    std::cerr << "Failed to build audio format" << std::endl;
    return;
  }

  pw_registry_add_listener(registry, &registry_listener, &registry_events,
                           this);
}

void AudioControl::openStreams() {
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    std::string playback_name =
        AuthConfig::instance().getPlaybackDeviceName();

    std::string capture_name =
        AuthConfig::instance().getCaptureDeviceName();

    uint32_t playback_id = PW_ID_ANY;
    uint32_t capture_id = PW_ID_ANY;

    if (playback_name != "auto") {
        if (!playback_devices.count(playback_name)) {
            std::cerr << "Unknown playback device\n";
            return;
        }

        playback_id = playback_devices[playback_name];
    }

    if (capture_name != "auto") {
        if (!capture_devices.count(capture_name)) {
            std::cerr << "Unknown capture device\n";
            return;
        }

        capture_id = capture_devices[capture_name];
    }

    auto *payload = new StreamInitData{
        this,
        capture_id,
        playback_id
    };

    pw_loop_invoke(
        pw_main_loop_get_loop(main_loop),

        [](spa_loop *loop,
           bool async,
           uint32_t seq,
           const void *data,
           size_t size,
           void *user_data) -> int {

            auto *p = static_cast<const StreamInitData *>(data);

            p->ctx->init_capture(p->capture_id);
            p->ctx->init_playback(p->playback_id);

            // delete p;

            return 0;
        },

        0,
        payload,
        sizeof(StreamInitData),
        false,
        nullptr
    );
}

bool AudioControl::init_playback(uint32_t devId = PW_ID_ANY) {
  struct pw_properties *playback_props = pw_properties_new(NULL);
  if (!playback_props) {
    std::cerr << "Failed to create playback properties\n";
    return false;
  }

  playback_stream =
      pw_stream_new_simple(pw_main_loop_get_loop(main_loop), "Playback",
                           playback_props, &playback_events, this);

  if (!playback_stream) {
    std::cerr << "Failed to create playback stream\n";
    pw_properties_free(playback_props);
    return false;
  }

  int res = pw_stream_connect(playback_stream, PW_DIRECTION_OUTPUT, devId,
                              (pw_stream_flags)(PW_STREAM_FLAG_AUTOCONNECT |
                                                PW_STREAM_FLAG_MAP_BUFFERS),
                              params, 1);

  if (res < 0) {
    std::cerr << "Failed to connect playback stream " << res << std::endl;
    return false;
  }

  return true;
}

bool AudioControl::init_capture(uint32_t devId = PW_ID_ANY) {
  struct pw_properties *capture_props = pw_properties_new(NULL);

  if (!capture_props) {
    std::cerr << "Failed to create capture properties\n";
    return false;
  }

  capture_stream =
      pw_stream_new_simple(pw_main_loop_get_loop(main_loop), "Capture",
                           capture_props, &capture_events, this);

  if (!capture_stream) {
    std::cerr << "Failed to create capture stream\n";
    pw_properties_free(capture_props);
    return false;
  }

  int res = pw_stream_connect(capture_stream, PW_DIRECTION_INPUT, devId,
                              (pw_stream_flags)(PW_STREAM_FLAG_AUTOCONNECT |
                                                PW_STREAM_FLAG_MAP_BUFFERS),
                              params, 1);

  if (res < 0) {
    std::cerr << "Failed to connect capture stream " << res << std::endl;
    return false;
  }

  return true;
}

void AudioControl::queue_audio(std::vector<uint8_t> &data,
                               bool waitForResponse) {}

void AudioControl::process_input(void *data) {
  AudioControl *ctx = (AudioControl *)data;
  struct pw_buffer *b;
  if (!(b = pw_stream_dequeue_buffer(ctx->capture_stream)))
    return;
  Ringbuffer<float> *input_buffer = ctx->input_buffer;
  if (input_buffer == nullptr) {
    std::cerr << "no input_buffer set\n";
    return;
  }

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

void AudioControl::process_output(void *data) {
  AudioControl *ctx = (AudioControl *)data;
  struct pw_buffer *b;
  if (!(b = pw_stream_dequeue_buffer(ctx->playback_stream)))
    return;

  struct spa_buffer *buf = b->buffer;
  float *dst = (float *)buf->datas[0].data;
  uint32_t n_frames = buf->datas[0].maxsize / sizeof(float);
  Ringbuffer<float> *output_buffer = ctx->output_buffer;

  if (output_buffer == nullptr) {
    std::cerr << "No output_buffer set" << std::endl;
    return;
  }

  for (uint32_t i = 0; i < n_frames; i++) {
    float sample = 0.0f;

    if (output_buffer->size() > 0) {
      output_buffer->pop(sample);
    }
    dst[i] = sample;
  }

  buf->datas[0].chunk->offset = 0;
  buf->datas[0].chunk->size = n_frames * sizeof(float);
  buf->datas[0].chunk->stride = sizeof(float);
  pw_stream_queue_buffer(ctx->playback_stream, b);
}

void AudioControl::setInputBuffer(Ringbuffer<float> *input_buffer) {
  this->input_buffer = input_buffer;
}

void AudioControl::setOutputBuffer(Ringbuffer<float> *output_buffer) {
  this->output_buffer = output_buffer;
}

void AudioControl::start_loop() {
  if (!main_loop || is_running)
    return;

  is_running = true;
  loop_thread = new std::thread([this]() { pw_main_loop_run(main_loop); });
}

void AudioControl::end_loop() {
  if (!main_loop || !is_running)
    return;

  is_running = false;
  pw_main_loop_quit(main_loop);

  if (loop_thread && loop_thread->joinable()) {
    loop_thread->join();
  }

  delete loop_thread;
  loop_thread = nullptr;
}

AudioControl::~AudioControl() {
  end_loop();

  if (registry) {
    spa_hook_remove(&registry_listener);
    pw_proxy_destroy((pw_proxy *)registry);
  }

  if (playback_stream)
    pw_stream_destroy(playback_stream);

  if (capture_stream)
    pw_stream_destroy(capture_stream);

  if (core)
    pw_core_disconnect(core);

  if (context)
    pw_context_destroy(context);

  if (main_loop)
    pw_main_loop_destroy(main_loop);

  pw_deinit();
}

void AudioControl::registry_event(void *data, uint32_t id, uint32_t permissions,
                                  const char *type, uint32_t version,
                                  const struct spa_dict *props) {

  if (strcmp(type, PW_TYPE_INTERFACE_Node) != 0)
    return;

  AudioControl *a = (AudioControl *)data;
  const char *media_class = spa_dict_lookup(props, PW_KEY_MEDIA_CLASS);

  const char *node_name = spa_dict_lookup(props, PW_KEY_NODE_DESCRIPTION);

  if (!media_class || !node_name)
    return;

  if (strcmp(media_class, "Audio/Sink") == 0) {
    a->playback_devices.insert({node_name, id});
    // std::cout << "New playback device" << std::endl;
    // std::cout << "[Playback] ";
  } else if (strcmp(media_class, "Audio/Source") == 0) {
    a->capture_devices.insert({node_name, id});
    // std::cout << "New capture device" << std::endl;
    // std::cout << "[Capture] ";
  } else {
    return;
  }

  // std::cout << "ID=" << id << " Name=" << node_name << std::endl;
}
