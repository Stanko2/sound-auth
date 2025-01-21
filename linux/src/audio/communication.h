#pragma once
#include "../ggwave/ggwave.h"
#include <vector>
#include "audio-control.h"

#define DEFAULT_RATE     48000

class AudioControl;

class Communication
{
private:
    GGWave* ggWave;
    std::vector<uint8_t> received_data;
    std::vector<uint8_t> waveform;

public:
    inline static Communication* instance;
    void samples_received(uint8_t* samples, size_t samples_size);
    int get_data(std::vector<uint8_t> &out);
    int encode_message(std::vector<uint8_t> &message);
    Communication (AudioControl* audio);
    std::vector<uint8_t> get_waveform();
    ~Communication();
};

// Communication* Communication::instance = NULL;
