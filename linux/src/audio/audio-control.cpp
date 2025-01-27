#include<iostream>
#include "audio-control.h"
#include <chrono>
#include <thread>

#define PI 3.14159265

template <class T>
float getTime_ms(const T & tStart, const T & tEnd) {
    return ((float)(std::chrono::duration_cast<std::chrono::microseconds>(tEnd - tStart).count()))/1000.0;
}

GGWave::SampleFormat AudioControl::getInputSampleFormat(){
  GGWave::SampleFormat ret = GGWAVE_SAMPLE_FORMAT_UNDEFINED;
  switch (captureSpec.format) {
    case AUDIO_U8:      ret = GGWAVE_SAMPLE_FORMAT_U8;  break;
    case AUDIO_S8:      ret = GGWAVE_SAMPLE_FORMAT_I8;  break;
    case AUDIO_U16SYS:  ret = GGWAVE_SAMPLE_FORMAT_U16; break;
    case AUDIO_S16SYS:  ret = GGWAVE_SAMPLE_FORMAT_I16; break;
    case AUDIO_S32SYS:  ret = GGWAVE_SAMPLE_FORMAT_F32; break;
    case AUDIO_F32SYS:  ret = GGWAVE_SAMPLE_FORMAT_F32; break;
  }

  return ret;
}

GGWave::SampleFormat AudioControl::getOutputSampleFormat(){
  GGWave::SampleFormat ret = GGWAVE_SAMPLE_FORMAT_UNDEFINED;
  switch (playbackSpec.format) {
    case AUDIO_U8:      ret = GGWAVE_SAMPLE_FORMAT_U8;  break;
    case AUDIO_S8:      ret = GGWAVE_SAMPLE_FORMAT_I8;  break;
    case AUDIO_U16SYS:  ret = GGWAVE_SAMPLE_FORMAT_U16; break;
    case AUDIO_S16SYS:  ret = GGWAVE_SAMPLE_FORMAT_I16; break;
    case AUDIO_S32SYS:  ret = GGWAVE_SAMPLE_FORMAT_F32; break;
    case AUDIO_F32SYS:  ret = GGWAVE_SAMPLE_FORMAT_F32; break;
  }

  return ret;
}

int AudioControl::getInputSampleRate() {
  return captureSpec.freq;
}

int AudioControl::getOutputSampleRate() {
  return playbackSpec.freq;
}

AudioControl::AudioControl()
{
  SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO);

  if (SDL_Init(SDL_INIT_AUDIO) < 0) {
    std::cerr << "Failed to initialize SDL" << std::endl;
    return;
  }

  SDL_SetHintWithPriority(SDL_HINT_AUDIO_RESAMPLING_MODE, "medium", SDL_HINT_OVERRIDE);
  // get playback devices
  int nDevices = SDL_GetNumAudioDevices(0);
  printf("Found %d playback devices:\n", nDevices);
  for (int i = 0; i < nDevices; i++) {
      printf("    - Playback device #%d: '%s'\n", i, SDL_GetAudioDeviceName(i, SDL_FALSE));
  }

  // get capture devices
  nDevices = SDL_GetNumAudioDevices(1);
  printf("Found %d capture devices:\n", nDevices);
  for (int i = 0; i < nDevices; i++) {
    printf("    - Capture device #%d: '%s'\n", i, SDL_GetAudioDeviceName(i, SDL_TRUE));
  }

  init_capture(0);
  init_playback(0);

}

bool AudioControl::init_playback(int devId) {
  std::cout << "Initializing playback" << std::endl;

  SDL_zero(playbackSpec);

  playbackSpec.freq = GGWave::kDefaultSampleRate;
  playbackSpec.format = AUDIO_S16SYS;
  playbackSpec.channels = 1;
  playbackSpec.samples = 16*1024;
  playbackSpec.callback = NULL;

  SDL_AudioSpec obtained;
  SDL_AudioDeviceID outDevice;

  SDL_zero(obtained);

  if (devId >= 0) {
    std::cout << "Using playback device: " << SDL_GetAudioDeviceName(devId, SDL_FALSE) << std::endl;
    outDevice = SDL_OpenAudioDevice(SDL_GetAudioDeviceName(devId, SDL_FALSE), SDL_FALSE, &playbackSpec, &obtained, 0);
  } else {
    outDevice = SDL_OpenAudioDevice(NULL, SDL_FALSE, &playbackSpec, &obtained, 0);
  }

  if (!outDevice) {
    std::cerr << "Failed to open audio device " << SDL_GetError() << std::endl;
    return false;
  }

  if (obtained.format != playbackSpec.format ||
    obtained.channels != playbackSpec.channels ||
    obtained.samples != playbackSpec.samples) {
    SDL_CloseAudio();
    std::cerr << "Requested audio format unsupported" << std::endl;
    return false;
  }

  playbackDevice = outDevice;

  printf("Obtained spec for output device (SDL Id = %d):\n", playbackDevice);
  printf("    - Sample rate:       %d\n", obtained.freq);
  printf("    - Format:            %d (required: %d)\n", obtained.format, playbackSpec.format);
  printf("    - Channels:          %d (required: %d)\n", obtained.channels, playbackSpec.channels);
  printf("    - Samples per frame: %d\n", obtained.samples);


  return true;
}

bool AudioControl::init_capture(int devId) {
  std::cout << "Initializing capture" << std::endl;

  SDL_zero(captureSpec);

  captureSpec.freq = GGWave::kDefaultSampleRate;
  captureSpec.format = AUDIO_F32SYS;
  captureSpec.channels = 1;
  captureSpec.samples = 1024;

  SDL_AudioSpec obtained;
  SDL_zero(obtained);
  SDL_AudioDeviceID captureDevice;

  if (devId >= 0) {
    std::cout << "using capture device " << SDL_GetAudioDeviceName(devId, SDL_TRUE) << std::endl;
    captureDevice = SDL_OpenAudioDevice(SDL_GetAudioDeviceName(devId, SDL_TRUE), SDL_TRUE, &captureSpec, &obtained, 0);
  } else {
    captureDevice = SDL_OpenAudioDevice(NULL, SDL_TRUE, &captureSpec, &obtained, 0);
  }

  if(!captureDevice) {
    std::cerr << "Failed to open audio capture device" << std::endl;
    return false;
  }

  this->captureDevice = captureDevice;

  printf("Obtained spec for input device (SDL Id = %d):\n", captureDevice);
  printf("    - Sample rate:       %d\n", obtained.freq);
  printf("    - Format:            %d (required: %d)\n", obtained.format, captureSpec.format);
  printf("    - Channels:          %d (required: %d)\n", obtained.channels, captureSpec.channels);
  printf("    - Samples per frame: %d\n", obtained.samples);


  return true;
}


void AudioControl::setRequiredBufferSize(size_t size) {
  required_buffer_size = size;
  output_buffer = malloc(500*required_buffer_size);
}

void AudioControl::queue_audio(std::vector<uint8_t> &data) {
  output_buffer_size = data.size();
  memcpy(output_buffer, data.data(), data.size());
}

bool AudioControl::loop_step(){
  if (!playbackDevice || !captureDevice) {
    std::cerr << "Trying to run loop without initialization" << std::endl;
    return false;
  }

  // std::cout << required_buffer_size << std::endl;

  if (!required_buffer_size) {
    std::cerr << "Trying to run loop buffer size" << std::endl;
    return false;
  }

  auto tNow = std::chrono::high_resolution_clock::now();

  if (output_buffer_size > 0) {
    SDL_PauseAudioDevice(captureDevice, SDL_TRUE);
    SDL_QueueAudio(playbackDevice, output_buffer, output_buffer_size);
    float duration = (float)output_buffer_size / (float)playbackSpec.freq;
    std::cout << "Duration" << duration << std::endl;
    output_buffer_size = 0;
    output_buffer = NULL;
    SDL_PauseAudioDevice(playbackDevice, SDL_FALSE);
  } else {
    static auto tLastNoData = std::chrono::high_resolution_clock::now();


    if (SDL_GetQueuedAudioSize(playbackDevice) < required_buffer_size) {
      SDL_PauseAudioDevice(playbackDevice, SDL_TRUE);
      SDL_PauseAudioDevice(captureDevice, SDL_FALSE);
      const int nHave = (int) SDL_GetQueuedAudioSize(captureDevice);
      const int nNeed = required_buffer_size;
      if (::getTime_ms(tLastNoData, tNow) > 500.0f && nHave >= nNeed) {
        if (capture_callback == NULL) {
          std::cerr << "No capture callback specified" << std::endl;
          return false;
        }

        uint8_t buffer[required_buffer_size];
        SDL_DequeueAudio(captureDevice, buffer, nNeed);
        capture_callback(buffer, required_buffer_size);
        if (nHave > 32*nNeed) {
          fprintf(stderr, "Warning: slow processing, clearing queued audio buffer of %d bytes ...\n", SDL_GetQueuedAudioSize(captureDevice));
          SDL_ClearQueuedAudio(captureDevice);
        }

      }
    } else {
      tLastNoData = tNow;
    }
  }

  return true;
}

void AudioControl::start_loop() {
  is_running = true;

  this->loop_thread = new std::thread([this](){
    loop();
  });

  this->loop_thread->join();
}

void AudioControl::end_loop() {
  is_running = false;
}

void AudioControl::loop() {
  while (is_running)
  {
    std::this_thread::sleep_for(std::chrono::microseconds(1));
    loop_step();
  }
}



AudioControl::~AudioControl() {
  SDL_PauseAudioDevice(captureDevice, 1);
  SDL_PauseAudioDevice(playbackDevice, 1);
  SDL_CloseAudioDevice(captureDevice);
  SDL_CloseAudioDevice(playbackDevice);

  free(output_buffer);
}
