#pragma once
#include <pipewire/pipewire.h>

class AudioControl {
private: 
    float* data;
    pw_main_loop *loop;
    pw_context *context;
    pw_core *core;
    pw_registry *registry;
    spa_hook *listener;
public:
    AudioControl(int argc, char *argv[]);
    void play(float* data);
    ~AudioControl();
};