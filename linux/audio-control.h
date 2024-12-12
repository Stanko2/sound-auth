#pragma once
#include <vector>

#include "communication.h"
#include <SDL2/SDL.h>

#include<thread>


#define DEFAULT_CHANNELS 2
#define DEFAULT_RATE     48000

class AudioControl {
private: 
    float* data;
    volatile bool is_running = false;
    inline static AudioControl* instance;
    size_t required_buffer_size = 0;

    void* output_buffer;
    size_t output_buffer_size = 0;

    static void process_output(void* data);
    static void process_input(void* data);
    bool init_playback(int devId);
    bool init_capture(int devId);

    bool loop_step();
    void loop();

    Communication* comm;
    SDL_AudioSpec playbackSpec;
    SDL_AudioSpec captureSpec;
    SDL_AudioDeviceID playbackDevice;
    SDL_AudioDeviceID captureDevice;

    std::thread* loop_thread;
public:

    void (*capture_callback)(uint8_t* data, size_t data_size) = NULL;
    GGWave::SampleFormat getOutputSampleFormat();
    GGWave::SampleFormat getInputSampleFormat();
    int getOutputSampleRate();
    int getInputSampleRate();
    void setRequiredBufferSize(size_t size);
    void start_loop();
    void end_loop();

    void queueAudio(std::vector<uint8_t> &data);

    AudioControl();
    ~AudioControl();
};