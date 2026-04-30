#include "audio-control.h"
#include "../config.h"
#include "pipewire/core.h"
#include "pipewire/main-loop.h"
#include "pipewire/pipewire.h"
#include "pipewire/port.h"
#include "pipewire/stream.h"
#include "spa/param/audio/raw-utils.h"
#include "spa/param/audio/raw.h"
#include "spa/param/param.h"
#include "spa/pod/builder.h"
#include "../sound-transfer-lib/RingBuffer.h"
#include <cstdint>
#include <iostream>

GGWave::SampleFormat AudioControl::getInputSampleFormat() {

  return GGWAVE_SAMPLE_FORMAT_F32;
}

GGWave::SampleFormat AudioControl::getOutputSampleFormat() {
  return GGWAVE_SAMPLE_FORMAT_F32;
}

int AudioControl::getInputSampleRate() { return 48000; }

int AudioControl::getOutputSampleRate() { return 48000; }

void AudioControl::listAllDevices() {
    //TODO
  // get playback devices
  // int nDevices = SDL_GetNumAudioDevices(0);
  // printf("Found %d playback devices:\n", nDevices);
  // for (int i = 0; i < nDevices; i++) {
  //   printf("    - Playback device #%d: '%s'\n", i,
  //          SDL_GetAudioDeviceName(i, SDL_FALSE));
  // }

  // // get capture devices
  // nDevices = SDL_GetNumAudioDevices(1);
  // printf("Found %d capture devices:\n", nDevices);
  // for (int i = 0; i < nDevices; i++) {
  //   printf("    - Capture device #%d: '%s'\n", i,
  //          SDL_GetAudioDeviceName(i, SDL_TRUE));
  // }
}

AudioControl::AudioControl() {
  pw_init(NULL, NULL);
  main_loop = pw_main_loop_new(NULL);

  struct spa_audio_info_raw info =
      SPA_AUDIO_INFO_RAW_INIT(.format = SPA_AUDIO_FORMAT_F32, .rate = 48000,
                              .channels = 1);

  struct spa_pod_builder b =
      SPA_POD_BUILDER_INIT(audio_buffer, sizeof(audio_buffer));
  params[0] = spa_format_audio_raw_build(&b, SPA_PARAM_EnumFormat, &info);

  init_capture(AuthConfig::instance().getCaptureDeviceId());
  init_playback(AuthConfig::instance().getPlaybackDeviceId());
}

bool AudioControl::init_playback(uint32_t devId = PW_ID_ANY) {
  struct pw_properties *playback_props = pw_properties_new(NULL);

  playback_stream =
      pw_stream_new_simple(pw_main_loop_get_loop(main_loop), "Playback",
                           playback_props, &playback_events, this);

  pw_stream_connect(playback_stream, PW_DIRECTION_OUTPUT, devId,
                    (pw_stream_flags)(PW_STREAM_FLAG_AUTOCONNECT |
                                      PW_STREAM_FLAG_MAP_BUFFERS),
                    params, 1);

  return true;
}

bool AudioControl::init_capture(uint32_t devId = PW_ID_ANY) {
  struct pw_properties *capture_props = pw_properties_new(NULL);

  capture_stream =
      pw_stream_new_simple(pw_main_loop_get_loop(main_loop), "Playback",
                           capture_props, &playback_events, this);

  pw_stream_connect(capture_stream, PW_DIRECTION_INPUT, devId,
                    (pw_stream_flags)(PW_STREAM_FLAG_AUTOCONNECT |
                                      PW_STREAM_FLAG_MAP_BUFFERS),
                    params, 1);

  return true;
}

void AudioControl::queue_audio(std::vector<uint8_t> &data,
                               bool waitForResponse) {

}

void AudioControl::process_input(void *data) {
    AudioControl *ctx = (AudioControl *)data;
    struct pw_buffer *b;
    if (!(b = pw_stream_dequeue_buffer(ctx->capture_stream)))
      return;
    Ringbuffer<float> *input_buffer = ctx->input_buffer;
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

void AudioControl::process_output(void *data) {
    AudioControl *ctx = (AudioControl *)data;
    struct pw_buffer *b;
    if (!(b = pw_stream_dequeue_buffer(ctx->playback_stream)))
      return;

    struct spa_buffer *buf = b->buffer;
    float *dst = (float *)buf->datas[0].data;
    uint32_t n_frames = buf->datas[0].maxsize / sizeof(float);

    for (uint32_t i = 0; i < n_frames; i++) {
      float sample = 0.0f;
      Ringbuffer<float> *output_buffer = ctx->output_buffer;
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

// bool AudioControl::loop_step() {
//   if (!playbackDevice || !captureDevice) {
//     std::cerr << "Trying to run loop without initialization" << std::endl;
//     return false;
//   }

//   if (!required_buffer_size) {
//     std::cerr << "Trying to run without loop buffer size" << std::endl;
//     return false;
//   }

//   // we have some data to send
//   if (output_buffer_size > 0) {
//     float duration = (float)output_buffer_size / (float)playbackSpec.freq;
//     // std::cout << "Sending message, duration: " << duration << "s" <<
//     // std::endl;
//     SDL_QueueAudio(playbackDevice, output_buffer, output_buffer_size);
//     output_buffer_size = 0;
//     output_buffer = NULL;
//   } else {

//     // still sending data, need to wait for it to finish
//     if (SDL_GetQueuedAudioSize(playbackDevice) > 0) {
//       SDL_ClearQueuedAudio(captureDevice);
//       SDL_Delay(10);

//       // no data to send, we can receive
//     } else {
//       const int nHave = (int)SDL_GetQueuedAudioSize(captureDevice);
//       const int nNeed = required_buffer_size;
//       if (nHave >= nNeed) {
//         if (capture_callback == NULL) {
//           std::cerr << "No capture callback specified" << std::endl;
//           return false;
//         }
//         std::vector<uint8_t> buffer(required_buffer_size);
//         SDL_DequeueAudio(captureDevice, buffer.data(), nNeed);
//         capture_callback(buffer.data(), required_buffer_size);
//         if (nHave > 32 * nNeed) {
//           std::cerr
//               << "Warning: slow processing, clearing queued audio buffer of "
//               << SDL_GetQueuedAudioSize(captureDevice) << " bytes";
//           SDL_ClearQueuedAudio(captureDevice);
//         }
//       }
//       SDL_Delay(10);
//     }
//   }

//   return true;
// }

void AudioControl::start_loop() {
  // std::cout << "Starting loop with timeout: " << timeout << std::endl;
  std::thread audio_thread([this]() {
    std::cout << "audio-thread started" << std::endl;
    pw_main_loop_run(main_loop);
  });
  // SDL_PauseAudioDevice(captureDevice, 0);
  // SDL_PauseAudioDevice(playbackDevice, 0);
  // if (timeout > 0) {
  //   loop(timeout);
  //   end_loop();
  // } else {
  //   loop();
  // }
}

void AudioControl::end_loop() {
  if (main_loop != NULL) {
      pw_main_loop_quit(main_loop);
      main_loop = NULL;
  }
}

// void AudioControl::loop(int timeout) {
//   std::chrono::time_point<std::chrono::steady_clock> start =
//       std::chrono::steady_clock::now();
//   while (is_running) {
//     long long elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
//                             std::chrono::steady_clock::now() - start)
//                             .count();
//     if (elapsed > timeout && timeout > 0) {
//       std::cerr << "Timeout reached: elapsed:" << elapsed
//                 << ", timeout: " << timeout << std::endl;
//       break;
//     }
//     if (!loop_step()) {
//       break;
//     }
//   }
// }

AudioControl::~AudioControl() {

}
