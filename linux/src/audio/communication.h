#pragma once
#include "../ggwave/ggwave.h"
#include <functional>
#include <vector>
#include "audio-control.h"

#define DEFAULT_RATE     48000
#define BROADCAST_ADDRESS new uint8_t[2] {0x00, 0x00}

class AudioControl;

class Communication
{
private:
    GGWave* ggWave;
    AudioControl* audio;
    std::vector<uint8_t> received_data;
    std::vector<uint8_t> waveform;
    char address[2];
    ggwave_ProtocolId protocol_id;
    bool is_valid(GGWave::TxRxData& data);
    int encode_message(std::vector<uint8_t> &message);
    std::vector<uint8_t> get_waveform();

public:
    void stop();
    void samples_received(uint8_t* samples, std::size_t samples_size);
    int get_data(std::vector<uint8_t> &out);
    int send_message(std::vector<uint8_t> &message, const uint8_t to[2], bool wait = true);
    int send_broadcast(std::vector<uint8_t> &message, bool wait = true);
    Communication (AudioControl* audio, const uint8_t address[2]);
    ~Communication();
    std::function<void(void)> receive_callback = NULL;
};
