#pragma once
extern "C" {
    #include <spa/param/audio/format-utils.h>
    #include <pipewire/pipewire.h>
}


#define DEFAULT_CHANNELS 2
#define DEFAULT_RATE     44100

class AudioControl {
private: 
    float* data;
    pw_main_loop *loop;
    pw_stream *output_stream;
    pw_context *context;
    pw_core *core;
    spa_hook *listener;
    uint8_t buffer[1024];
    pw_stream_events stream_events
    {
        PW_VERSION_STREAM_EVENTS,
        .process=process
    };
    static void process(void*data);
public:
    AudioControl(int argc, char *argv[]);
    void play(uint8_t* data);
    ~AudioControl();
};