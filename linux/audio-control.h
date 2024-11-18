#pragma once
extern "C" {
    #include <spa/param/audio/format-utils.h>
    #include <pipewire/pipewire.h>
}

#include <vector>

#include "communication.h"

#define DEFAULT_CHANNELS 2
#define DEFAULT_RATE     44100

class AudioControl {
private: 
    float* data;
    inline static AudioControl* instance;
    pw_main_loop *loop;
    pw_stream *output_stream;
    pw_stream *input_stream;
    pw_context *context;
    pw_core *core;
    spa_hook *listener;
    uint8_t output_buffer[1024];
    uint8_t input_buffer[1024];
    
    pw_stream_events output_stream_events
    {
        PW_VERSION_STREAM_EVENTS,
        .process=process_output
    };

    pw_stream_events input_stream_events
    {
        PW_VERSION_STREAM_EVENTS,
        .process=process_input
    };

    static void process_output(void* data);
    static void process_input(void* data);

    void on_samples_received();
    Communication* comm;
public:
    AudioControl(Communication* comm);
    void play(uint8_t* data);
    ~AudioControl();
};

struct process_data {
    pw_stream *stream;
    GGWave* ggwave;
};
