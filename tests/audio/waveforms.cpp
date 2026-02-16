#pragma once
#include <algorithm>
#include <cassert>
#include <cmath>
#include <complex>
#include <cstddef>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <fftw3.h>

#define PI 3.14159265358979323846
typedef std::vector<std::complex<float>> spectrum;


struct Peak {
    float frequency;
    float amplitude;
    float phase;
};

std::ostream& operator<<(std::ostream& os, const Peak& p) {
    os << "Peak{freq: " << p.frequency << ", amp: " << p.amplitude << ", phase: " << p.phase << "}";
    return os;
}

std::ostream& operator<<(std::ostream& os, const std::vector<float>& p) {
    for (auto &f : p) {
        os << f << ' ';
    }
    os << std::endl;
    return os;
}

/*
 * Creates a waveform that can be written directly to speaker
 * Frequencies - which frequencies should be mixed
 * Amplitudes - amplitude of each frequency
 */
std::vector<float> createWaveform(const std::vector<float>& frequencies, const std::vector<float>& amplitudes, const std::vector<float> phases, int sample_rate, float duration) {
    int N = static_cast<int>(duration * (float)sample_rate);
    std::vector<float> waveform(N,0);
    assert(frequencies.size() == amplitudes.size());
    assert(amplitudes.size() == phases.size());

    std::cout << frequencies << amplitudes << phases;


    for (int i = 0; i < N; i++) {
        double t = (double)i / (double)sample_rate;
        double acc = 0;
        for (int j = 0; j < frequencies.size(); j++) {
            float f = frequencies[j];
            float a = amplitudes[j];
            float p = phases[j];
            acc += a * sin(2 * PI * (double)f * t + p);
        }
        waveform[i] = static_cast<float>(acc);
    }

    // apply fadeIn / fadeOut to prevent "clicks"
    int fadeSamples = (int)(0.002f * (float)sample_rate);
    for (int i = 0; i < fadeSamples; i++) {
        waveform[i] *= i / (float)fadeSamples;
    }

    for (int i = 0; i < fadeSamples; i++) {
        waveform[waveform.size() - 1 - i] *= i / (float)fadeSamples;
    }

    return waveform;
}

void normalize(std::vector<float>& waveform) {
    float max_val = 0;
    for(float x : waveform) {
        max_val = std::max(max_val, std::abs(x));
    }
    if (max_val <= 0)
        return;
    for(float& s : waveform) {
        s /= max_val;
    }
}

std::vector<std::string> split(const std::string& s, const char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(s);
    std::string token;
    while(std::getline(ss,token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

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
std::vector<float> getWaveform(const std::string data, const int samples_per_frame, const int sample_rate) {
    std::vector<float> out;
    float frame_duration = static_cast<float>(samples_per_frame) / static_cast<float>(sample_rate);
    std::vector<float> frequencies;
    std::vector<float> amplitudes;
    std::vector<float> phases;

    for (auto &i : split(data, '|')) {
        frequencies.clear();
        amplitudes.clear();
        phases.clear();
        std::cout << i << std::endl;
        for (auto &j : split(i, ',')) {
            std::vector<std::string> nums = split(j, ':');
            frequencies.push_back(atof(nums[0].c_str()));
            if (nums.size() > 1) {
                amplitudes.push_back(atof(nums[1].c_str()));
            } else {
                amplitudes.push_back(1);
            }
            if (nums.size() > 2) {
                phases.push_back(atof(nums[2].c_str()));
            } else {
                phases.push_back(0);
            }
        }
        std::vector<float> frame_waveform = createWaveform(frequencies, amplitudes, phases, sample_rate, frame_duration);
        for (auto &f: frame_waveform) {
            out.push_back(f);
        }
        std::cout << "waveform size: " << out.size() << " " << frame_waveform[5] << std::endl;
    }

    return out;
}

spectrum get_spectrum(const std::vector<float>& waveform, int sample_rate) {
    fftw_complex *in, *out;
    fftw_plan p;
    size_t N = waveform.size();

    in = fftw_alloc_complex(N);
    float max_sample = 0;
    spectrum ret;
    for (size_t i = 0; i < N; i++) {
        float w = 0.5 * (1 - cos(2 * M_PI * i / (N - 1)));
        in[i][0] = w * waveform[i];
        max_sample = std::max(max_sample, (float)in[i][0]);
        in[i][1] = 0;
    }

    // std::cout << "Max sample: " << max_sample << std::endl;

    out = fftw_alloc_complex(N);

    p = fftw_plan_dft_1d(N, in, out, FFTW_FORWARD, FFTW_ESTIMATE);

    fftw_execute(p);

    for (size_t i = 0; i < N; i++) {
        ret.push_back({(float)out[i][0], (float)out[i][1]});
    }

    fftw_destroy_plan(p);
    fftw_free(in);
    fftw_free(out);

    return ret;
}

std::vector<Peak> get_peaks(spectrum spectrum, int sample_rate, int N) {

    std::vector<float> magnitudes(N/2);
    for (size_t i = 0; i < N / 2; ++i) {
        float real = spectrum[i].real();
        float imag = spectrum[i].imag();
        magnitudes[i] = sqrt(real * real + imag * imag);
    }

    double max_freq = 0;
    int max_bin = 0;
    double max_mag = 0;

    std::vector<Peak> peaks;
    peaks.clear();
    for (size_t i = 0; i < magnitudes.size(); ++i) {
        double freq = (double)i * sample_rate / N;
        if (freq < 10000) continue;
        // if (magnitudes[i] > max_mag) {
        //     max_bin = i;
        //     max_freq = freq;
        //     max_mag = magnitudes[i];
        // }
        if (magnitudes[i] < 30 || freq > 20000) continue;
        if (magnitudes[i] > magnitudes[i-1] && magnitudes[i] > magnitudes[i+1]) {
            Peak p = Peak();
            p.amplitude = magnitudes[i];
            p.frequency = freq;
            p.phase =  std::atan2(spectrum[i].imag(), spectrum[i].real());
            peaks.push_back(p);
        }
    }

    // std::cout << "freq: " << max_freq << " mag: " << max_mag;
    if (peaks.size() > 0) {
        std::cout << "frame: ";
        for (size_t i = 0; i < peaks.size(); i++) {
            std::cout << peaks[i] << " ";
        }
        std::cout << std::endl;
    }


    return peaks;
}
