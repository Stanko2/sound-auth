#include <cmath>
#include "filter.h"

CombFilter::CombFilter(float sample_rate, float target, float decay_seconds) {
    delaySamples = (int) (sample_rate / target);
    buffer.resize(delaySamples, 0.0f);
    writeIndex = 0;

    float d = std::max(0.001f, decay_seconds);
    gain = std::pow(0.001f, (float)delaySamples / (d * sample_rate));
}

float CombFilter::process(float input) {
    float delayed = buffer[writeIndex];
    float output = input + (gain * delayed);

    buffer[writeIndex] = output;

    writeIndex = (writeIndex + 1) % delaySamples;

    return output;
}
