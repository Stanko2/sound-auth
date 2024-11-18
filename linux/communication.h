#pragma once
#include <ggwave/ggwave.h>
#include <vector>

class Communication
{
private:
    GGWave* ggwave;
    std::vector<uint16_t> received_samples;
    std::vector<uint8_t> received_data;
    std::vector<uint16_t> queued_samples;
public:
    void samples_received(uint16_t* samples, size_t samples_size);
    int get_data(std::vector<uint8_t> &out);
    Communication(/* args */);
    ~Communication();
};