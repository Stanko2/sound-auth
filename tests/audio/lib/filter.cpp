#include "filter.h"

CombFilter::CombFilter(float sample_rate, float target, float g) {
    delaySamples = (int) (sample_rate / target);
    buffer.resize(delaySamples, 0);
    gain = g;
}

float CombFilter::process(float input) {
    float delayed = buffer[writeIndex];

    buffer[delayed] = input;

    writeIndex = (writeIndex + 1) % delaySamples;

    return gain * (input + delayed);
}
