#pragma once
#include "../sound-transfer-lib/RingBuffer.h"
#include "communication.h"
#include "spa/pod/pod.h"
#include <string>
#include <atomic>
#include <cstdint>
#include <pipewire/pipewire.h>
#include <thread>
#include <unordered_map>
#include <vector>

class AudioControl {
private:
  static AudioControl *instance;

  bool init_playback(uint32_t devId);
  bool init_capture(uint32_t devId);

  Ringbuffer<float> *output_buffer;
  Ringbuffer<float> *input_buffer;

  /* Pipewire stuff */

  uint8_t audio_buffer[1024];
  const struct spa_pod *params[1];

  struct pw_main_loop *main_loop;
  struct pw_stream *capture_stream;
  struct pw_stream *playback_stream;

  std::thread *loop_thread;
  std::atomic<bool> is_running{false};

  pw_context *context = nullptr;
  pw_core *core = nullptr;
  pw_registry *registry = nullptr;
  spa_hook registry_listener{};

  std::unordered_map<std::string, int> capture_devices;
  std::unordered_map<std::string, int> playback_devices;

public:
  static void process_output(void *data);
  static void process_input(void *data);
  static void registry_event(void *data, uint32_t id, uint32_t permissions,
                             const char *type, uint32_t version,
                             const struct spa_dict *props);

  void setInputBuffer(Ringbuffer<float> *input_buffer);
  void setOutputBuffer(Ringbuffer<float> *output_buffer);

  void listAllDevices();
  void openStreams();
  void (*capture_callback)(uint8_t *data, std::size_t data_size) = NULL;
  GGWave::SampleFormat getOutputSampleFormat();
  GGWave::SampleFormat getInputSampleFormat();
  int getOutputSampleRate();
  int getInputSampleRate();
  void setRequiredBufferSize(std::size_t size);
  void start_loop();
  void end_loop();
  void queue_audio(std::vector<uint8_t> &data, bool waitForResponse);

  AudioControl();
  ~AudioControl();
};

static void on_stream_state_changed(void *data, enum pw_stream_state old,
                                    enum pw_stream_state state,
                                    const char *error) {
  // printf("Stream state changed: %s -> %s\n", pw_stream_state_as_string(old),
  // pw_stream_state_as_string(state));
  if (state == PW_STREAM_STATE_ERROR) {
    fprintf(stderr, "Stream error: %s\n", error);
  }
}

static const struct pw_stream_events capture_events = {
    .version = PW_VERSION_STREAM_EVENTS,
    .state_changed = on_stream_state_changed,
    .process = AudioControl::process_input};
static const struct pw_stream_events playback_events = {
    .version = PW_VERSION_STREAM_EVENTS,
    .state_changed = on_stream_state_changed,
    .process = AudioControl::process_output};

static const pw_registry_events registry_events = {
    PW_VERSION_REGISTRY_EVENTS,
    .global = AudioControl::registry_event,
};

struct StreamInitData {
    AudioControl *ctx;
    uint32_t capture_id;
    uint32_t playback_id;
};
