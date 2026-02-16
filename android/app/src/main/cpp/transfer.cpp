
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cassert>
#include <iostream>
#include <vector>
#include <fftw3.h>
#include <jni.h>
#include <string>
#include <android/log.h>
#include <sstream>


#define PI 3.14159265358979323846

struct Peak {
    float frequency;
    float amplitude;
    float phase;
};

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
std::vector<float> createWaveform(const std::vector<float>& frequencies, const std::vector<float>& amplitudes, const std::vector<float>& phases, int sample_rate, float duration) {
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
    int fadeInSamples = (int)(0.002f * (float)sample_rate);
    for (int i = 0; i < fadeInSamples; i++) {
        waveform[i] *= (float)i / (float)fadeInSamples;
    }

    int fadeOutSamples = 2 * fadeInSamples;
    for (int i = 0; i < fadeOutSamples; i++) {
        waveform[waveform.size() - 1 - i] *= (float)i / (float)fadeOutSamples;
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
std::vector<float> getWaveform(const std::string& data, const int samples_per_frame, const int sample_rate) {
    std::vector<float> out;
    float frame_duration = static_cast<float>(samples_per_frame) / static_cast<float>(sample_rate) / 4;
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

std::string jstring2string(JNIEnv *env, jstring jStr) {
    if (!jStr)
        return "";

    const jclass stringClass = env->GetObjectClass(jStr);
    const jmethodID getBytes = env->GetMethodID(stringClass, "getBytes", "(Ljava/lang/String;)[B");
    const jbyteArray stringJbytes = (jbyteArray) env->CallObjectMethod(jStr, getBytes, env->NewStringUTF("UTF-8"));

    size_t length = (size_t) env->GetArrayLength(stringJbytes);
    jbyte* pBytes = env->GetByteArrayElements(stringJbytes, NULL);

    std::string ret = std::string((char *)pBytes, length);
    env->ReleaseByteArrayElements(stringJbytes, pBytes, JNI_ABORT);

    env->DeleteLocalRef(stringJbytes);
    env->DeleteLocalRef(stringClass);
    return ret;
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
                                                                     jstring input) {
    std::string s = jstring2string(env, input);
    __android_log_print(ANDROID_LOG_DEBUG, "SOUND", "input: %s", s.c_str());
    std::vector<float> output = getWaveform(s, 2048, 48000);

    normalize(output);

    jfloatArray ret = env->NewFloatArray((jsize)output.size());
    if (!output.empty()) {
        env->SetFloatArrayRegion(ret, 0, (jsize)output.size(), output.data());
    }
    return ret;
}