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
#include "waveforms.h"
#include "RingBuffer.h"
#include "filter.h"
#include <fstream>
#ifndef __ANDROID__
#include "wavExport.cpp"
#endif

Waveforms::Waveforms(int buffer_size, int frame_size) {
    receive_sample_buffer = new Ringbuffer<float>(buffer_size);
    filters.clear();
    this->frame_size = frame_size;
#ifndef __ANDROID__
    recordFile.open("data.wav", std::ios::binary);
#endif
}

std::vector<float> Waveforms::createWaveform(const std::vector<float>& frequencies, const std::vector<float>& amplitudes, const std::vector<float> phases, int sample_rate, float duration) {
    int N = static_cast<int>(duration * (float)sample_rate);
    // std::cout << "F: " << frequencies << std::endl;
    // std::cout << "A: " << amplitudes << std::endl;
    // std::cout << "P: " << phases << std::endl;
    // std::cout << "t: " << duration << std::endl;
    std::vector<float> waveform(N,0);
    assert(frequencies.size() == amplitudes.size());
    assert(amplitudes.size() == phases.size());


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
    int fadeSamples = (int)(0.0005f * (float)sample_rate);
    for (int i = 0; i < fadeSamples; i++) {
        waveform[i] *= i / (float)fadeSamples;
    }

    for (int i = 0; i < fadeSamples; i++) {
        waveform[waveform.size() - 1 - i] *= i / (float)fadeSamples;
    }

    return waveform;
}

void Waveforms::normalize(waveform& waveform) {
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

void Waveforms::addFilter(Filter* filter) {
    filters.push_back(filter);
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

waveform Waveforms::getWaveform(const std::string data, const int samples_per_frame, const int sample_rate) {
    waveform out;
    float frame_duration = static_cast<float>(samples_per_frame) / static_cast<float>(sample_rate);
    std::vector<float> frequencies;
    std::vector<float> amplitudes;
    std::vector<float> phases;

    for (auto &i : split(data, '|')) {
        frequencies.clear();
        amplitudes.clear();
        phases.clear();
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
        waveform frame_waveform = createWaveform(frequencies, amplitudes, phases, sample_rate, frame_duration);
        for (auto &f: frame_waveform) {
            out.push_back(f);
        }
        std::cout << "waveform size: " << out.size() << std::endl;
    }

    normalize(out);
    return out;
}

Spectrum* Waveforms::get_spectrum(const waveform& waveform, int sample_rate) {
#ifdef __ANDROID__
    fftwf_complex *in, *out;
    fftwf_plan p;
#else
    fftw_complex *in, *out;
    fftw_plan p;
#endif
    size_t N = waveform.size();

#if __ANDROID__
    in = fftwf_alloc_complex(N);
#else
    in = fftw_alloc_complex(N);
#endif

    float max_sample = 0;
    std::vector<std::complex<float>> ret;
    for (size_t i = 0; i < N; i++) {
        float w = 0.5 * (1 - cos(2 * PI * i / (N - 1)));
        in[i][0] = w * waveform[i];
        max_sample = std::max(max_sample, (float)in[i][0]);
        in[i][1] = 0;
    }

#if __ANDROID__
    out = fftwf_alloc_complex(N);
    p = fftwf_plan_dft_1d(N, in, out, FFTW_FORWARD, FFTW_ESTIMATE);
    fftwf_execute(p);
#else
    out = fftw_alloc_complex(N);
    p = fftw_plan_dft_1d(N, in, out, FFTW_FORWARD, FFTW_ESTIMATE);
    fftw_execute(p);
#endif

    for (size_t i = 0; i < N; i++) {
        ret.push_back({(float)out[i][0], (float)out[i][1]});
    }

#if __ANDROID__
    fftwf_destroy_plan(p);
    fftwf_free(in);
    fftwf_free(out);
#else
    fftw_destroy_plan(p);
    fftw_free(in);
    fftw_free(out);
#endif

    return new Spectrum(ret);
}

std::vector<Peak> Waveforms::get_peaks(Spectrum* spectrum, int sample_rate, int N) {

    std::vector<float> magnitudes(N/2);
    for (size_t i = 0; i < N / 2; ++i) {
        magnitudes[i] = spectrum->mag(i);
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
        if (magnitudes[i] < 1 || freq > 20000) continue;
        if (magnitudes[i] > magnitudes[i-1] && magnitudes[i] > magnitudes[i+1]) {
            Peak p = Peak();
            p.amplitude = magnitudes[i];
            p.frequency = freq;
            p.phase =  spectrum->phase(i);
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

waveform Waveforms::get_frame(int offset) {
    waveform ret;
    for (size_t i = 0; i < frame_size; i++) {
        ret.push_back(receive_sample_buffer->get(i+offset));
    }
    return ret;
}

void Waveforms::enqueue_frame(const std::vector<float>& samples) {
    std::vector<float> processed;
    for(float sample : samples) {
        for (Filter* f : filters) {
            sample = f->process(sample);
        }
        processed.push_back(sample);
        receive_sample_buffer->add(sample);
    }
    int len = samples.size() * sizeof(float);
    totalBytes += len;
    #ifndef __ANDROID__
    recordFile.write(reinterpret_cast<const char*>(processed.data()), len); // <- Pokazene
    #endif
}


Waveforms::~Waveforms() {
    #ifndef __ANDROID__
    std::cout << "Finalize wav" << std::endl;
    finalizeWav(recordFile, totalBytes, 1, 48000, 32);
    #endif
}

Spectrum::Spectrum(std::vector<std::complex<float>>& data) {
    this->data = data;
}

const float Spectrum::mag(const int f) {
    return sqrtf(data[f].real() * data[f].real() + data[f].imag() * data[f].imag());
}

const float Spectrum::strength(const int f) {
    float s = 0;
    for (int i = -10; i <= 10; i++) {
        s += mag(f);
    }

    return 20 * std::log10(s / (20 * data.size()));
}

const float Spectrum::phase(const int f) {
    return std::atan2(data[f].imag(), data[f].real());
}
