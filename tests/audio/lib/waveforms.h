#pragma once
#include <fstream>
#include <iostream>
#include <vector>
#include <complex>
#include <string>
#include "RingBuffer.h"
#include "filter.h"

#define PI 3.14159265358979323846

typedef std::vector<float> waveform;

struct Peak {
    float frequency;
    float amplitude;
    float phase;
};

inline std::ostream& operator<<(std::ostream& os, const Peak& p) {
    os << "Peak{freq: " << p.frequency << ", amp: " << p.amplitude << ", phase: " << p.phase << "}";
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const std::vector<float>& p) {
    for (auto &f : p) {
        os << f << ' ';
    }
    os << std::endl;
    return os;
}


// simple wrapper to spectrum
class Spectrum {
private:
    std::vector<std::complex<float>> data;
    float min_strength;
public:
    Spectrum(std::vector<std::complex<float>>& data, float min_strength);

    // get magnitude of frequency f in this spectrum
    const float mag(const int f);

    // get phase of frequency f
    const float phase(const int f);

    // get strength in dB
    const float strength(const int f);
};


// class to manage waveforms => frequencies
class Waveforms {
private:
    int frame_size;
    WindowFunction* window_function;
    Ringbuffer<float>* receive_sample_buffer;
    void normalize(waveform& waveform);
    std::vector<Filter*> filters;
    float applyFilters(float sample);


    std::ofstream recordFile;
    size_t totalBytes = 0;

    /*
    * Creates a waveform that can be written directly to speaker
    * Frequencies - which frequencies should be mixed
    * Amplitudes - amplitude of each frequency
    */
    std::vector<float> createWaveform(const std::vector<float>& frequencies, const std::vector<float>& amplitudes, const std::vector<float> phases, int sample_rate, float duration);
public:
    Waveforms(int buffer_size, int frame_size, WindowFunction* window_function);

    void addFilter(Filter* filter);


    /*
    * Frequencies representation:
    * - Frame divider: "|"
    * - Frequency divider: ","
    * - Amplitude & phase: ":"
    *
    * Play frequencies 16300 with 16000 and then 17000Hz:
    * "16300,16000|17000"
    */

    /*
    * Parses a data string and creates a waveform for it
    */
    std::vector<float> getWaveform(const std::string data, const int samples_per_frame, const int sample_rate);

    Spectrum* get_spectrum(const std::vector<float>& waveform, int sample_rate, float min_strength);

    /*
     * Tries to get peaks from the spectrum - old method, not used
     */
    std::vector<Peak> get_peaks(Spectrum* spectrum, int sample_rate, int N);

    /**
     * Get a continuous frame (of length p.N) starting at `offset` within the
     * sample_buffer. The returned vector contains `p.N` samples.
     *
     * - offset: index into sample_buffer where the frame begins
     */
    std::vector<float> get_frame(int offset);

    /*
     * Adds a new frame to the buffer
     */
    void enqueue_frame(const std::vector<float>& samples);

    ~Waveforms();
};
