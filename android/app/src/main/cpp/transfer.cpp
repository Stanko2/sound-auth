
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <vector>
#include <fftw3.h>
#include <jni.h>
#include <android/log.h>

#define PI 3.14159265358979323846

struct Peak {
    float frequency;
    float amplitude;
    float phase;
};

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

std::vector<Peak> getFrequencies(const std::vector<float>& waveform, int sample_rate) {
    fftwf_complex *in, *out;
    fftwf_plan p;
    size_t N = waveform.size();

    in = fftwf_alloc_complex(N);
    double max_sample = 0;
    for (size_t i = 0; i < N; i++) {
        max_sample = std::max(max_sample, (double)std::abs(waveform[i]));
    }

    // normalize volume - on Linux should do nothing
    // android has all samples ~2000x lower
    double scale = 1.0 / max_sample;
    for (size_t i = 0; i < N; i++) {
        // apply window function
        double w = 0.5 * (1 - cos(2 * M_PI * i / (N - 1)));
        in[i][0] = (float)(w * scale * waveform[i]);
        in[i][1] = 0;
    }

    out = fftwf_alloc_complex(N);

    p = fftwf_plan_dft_1d(N, in, out, FFTW_FORWARD, FFTW_ESTIMATE);

    fftwf_execute(p);

    std::vector<float> magnitudes(N/2);
    for (size_t i = 0; i < N / 2; ++i) {
        float real = out[i][0];
        float imag = out[i][1];
        magnitudes[i] = sqrt(real * real + imag * imag);
    }

    fftwf_destroy_plan(p);
    fftwf_free(in);
    fftwf_free(out);

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
        if (magnitudes[i] < 50 || freq > 20000) continue;
        if (magnitudes[i] > magnitudes[i-1] && magnitudes[i] > magnitudes[i+1]) {
            Peak peak{};
            peak.amplitude = magnitudes[i];
            peak.frequency = (float)freq;
            peak.phase = std::atan2(out[i][1], out[i][0]);
            peaks.push_back(peak);
            __android_log_print(ANDROID_LOG_DEBUG, "SOUND", "Detected Peak at %fHz: %f %f", freq, peak.amplitude, peak.phase);
        }
    }

    return peaks;
}


extern "C"
JNIEXPORT jfloatArray JNICALL
Java_com_example_soundauth_ui_SoundTestingScreen_RunFFT(JNIEnv *env, jobject thiz,
                                                        jfloatArray input) {
    size_t len = env->GetArrayLength(input);
    float* samples = env->GetFloatArrayElements(input, nullptr);
    std::vector<float> waveform {samples, samples + len};

    std::vector<Peak> output = getFrequencies(waveform, 48000);

    std::vector<float> freqs;
    for(auto &i : output) {
        freqs.push_back(i.frequency);
    }

    jfloatArray ret = env->NewFloatArray((jsize)output.size());
    if (!output.empty()) {
        env->SetFloatArrayRegion(ret, 0, (jsize)output.size(), freqs.data());
    }
    return ret;
}

extern "C"
JNIEXPORT jfloatArray JNICALL
Java_com_example_soundauth_ui_SoundTestingScreen_GenerateFrequencies(JNIEnv *env, jobject thiz,
                                                                     jfloatArray input) {
    size_t len = env->GetArrayLength(input);
    float* fInput = env->GetFloatArrayElements(input, nullptr);
    std::vector<float> frequencies {fInput, fInput + len};

    std::vector<float> output = createWaveform(frequencies, 48000, 5.0);

    normalize(output);

    jfloatArray ret = env->NewFloatArray((jsize)output.size());
    if (!output.empty()) {
        env->SetFloatArrayRegion(ret, 0, (jsize)output.size(), output.data());
    }
    return ret;
}