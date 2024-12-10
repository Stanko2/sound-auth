#pragma once
#include <ggwave/ggwave.h>
#include <vector>
#include "audio-control.h"

#define DEFAULT_RATE     48000

class AudioControl;

class Communication
{
private:
    GGWave* ggWave;
    std::vector<uint8_t> received_data;
public:
    static Communication* instance;
    void samples_received(uint8_t* samples, size_t samples_size);
    int get_data(std::vector<uint8_t> &out);
    Communication (AudioControl* audio);
    ~Communication();
};

Communication* Communication::instance = NULL;