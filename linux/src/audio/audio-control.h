#pragma once
#include <cstdint>
#include <pipewire/pipewire.h>
#include <vector>
#include "../sound-transfer-lib/RingBuffer.h"
#include "communication.h"
#include "spa/pod/pod.h"
#include<thread>




class AudioControl {
private:
    static AudioControl* instance;

    bool init_playback(uint32_t devId);
    bool init_capture(uint32_t devId);

    Ringbuffer<float>* output_buffer;
    Ringbuffer<float>* input_buffer;

    /* Pipewire stuff */

    uint8_t audio_buffer[1024];
    const struct spa_pod* params[1];

    struct pw_main_loop *main_loop;
    struct pw_stream *capture_stream;
    struct pw_stream *playback_stream;

    std::thread* loop_thread;
public:
    static void process_output(void* data);
    static void process_input(void* data);

    void listAllDevices();
    void (*capture_callback)(uint8_t* data, std::size_t data_size) = NULL;
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

static const struct pw_stream_events capture_events = {
    .version = PW_VERSION_STREAM_EVENTS,
    // .state_changed = on_stream_state_changed,
    .process = AudioControl::process_input};
static const struct pw_stream_events playback_events = {
    .version = PW_VERSION_STREAM_EVENTS,
    // .state_changed = on_stream_state_changed,
    .process = AudioControl::process_output};
