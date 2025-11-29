
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <vector>
#include <fftw3.h>

#define PI 3.14159265358979323846

std::vector<float> createWaveform(const std::vector<float>& frequencies, int sample_rate, float duration) {
    int N = static_cast<int>(duration * (float)sample_rate);
    std::vector<float> waveform(N,0);

    for (int i = 0; i < N; i++) {
        double t = (double)i / (double)sample_rate;
        double acc = 0;
        for (float f : frequencies) {
            acc += sin(2 * PI * (double)f * t);
        }
        waveform[i] = static_cast<float>(acc);
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

std::vector<float> getFrequencies(const std::vector<float>& waveform, int sample_rate) {
    fftw_complex *in, *out;
    fftw_plan p;
    size_t N = waveform.size();

    in = fftw_alloc_complex(N);
    for (size_t i = 0; i < N; i++) {
        float w = 0.5 * (1 - cos(2 * M_PI * i / (N - 1)));
        in[i][0] = w * waveform[i];
        in[i][1] = 0;
    }

    out = fftw_alloc_complex(N);

    p = fftw_plan_dft_1d(N, in, out, FFTW_FORWARD, FFTW_ESTIMATE);

    fftw_execute(p);

    std::vector<float> magnitudes(N/2);
    for (size_t i = 0; i < N / 2; ++i) {
        float real = out[i][0];
        float imag = out[i][1];
        magnitudes[i] = sqrt(real * real + imag * imag);
    }

    fftw_destroy_plan(p);
    fftw_free(in);
    fftw_free(out);

    double max_freq = 0;
    int max_bin = 0;
    double max_mag = 0;

    std::vector<float> peaks;
    peaks.clear();
    for (size_t i = 0; i < magnitudes.size(); ++i) {
        double freq = (double)i * sample_rate / N;
        if (freq < 10000) continue;
        // if (magnitudes[i] > max_mag) {
        //     max_bin = i;
        //     max_freq = freq;
        //     max_mag = magnitudes[i];
        // }
        if (magnitudes[i] < 5 || freq > 20000) continue;
        if (magnitudes[i] > magnitudes[i-1] && magnitudes[i] > magnitudes[i+1]) {
            peaks.push_back(freq);
        }
    }

    // std::cout << "freq: " << max_freq << " mag: " << max_mag;

    std::cout << "frame: ";
    for (size_t i = 0; i < peaks.size(); i++) {
        std::cout << peaks[i] << " ";
    }

    std::cout << std::endl;

    return peaks;
}
