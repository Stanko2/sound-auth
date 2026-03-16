#pragma once
#include <vector>

// Interface for audio filters
class Filter {
public:
    virtual ~Filter() = default;
    virtual float process(float input) = 0;
};

// delay L, output y, input x
// y[n] = x[n] + x[n-L]
class CombFilter : public Filter {
private:
    std::vector<float> buffer;
    int writeIndex = 0;
    int delaySamples;
    float gain;

public:
    CombFilter(float sample_rate, float target, float decay);
    float process(float input) override;
};
