#include <cassert>
#include <cmath>
#include <complex>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <deque>
#include <fstream>
#include <ios>
#include <iostream>
#include <vector>
#include "waveforms.cpp"
#define F1
#define F2


struct ProtocolConfig {
    int f1;
    int f2;
    int N;
    int sample_rate;
    float peak_threshold;
};

enum State {
    idle = 0,
    processing = 1,
};

std::deque<float> sample_buffer;
std::vector<uint8_t> rx_buffer;
std::vector<uint8_t> tx_buffer;
State recorder_state = idle;
int sync_offset = 0;
int msg_frames = 0;

void init(const ProtocolConfig& p) {
    sample_buffer = std::deque<float>(6*p.N);
}



// gets specific frame for offset
//
// - frame which frame of buffer to use (0-3)
// - offset from the original recorded frame
//
// returns: samples for frame [frame] at offset [offset]
std::vector<float> get_frame(const ProtocolConfig& p, int offset) {
    std::vector<float> ret;
    for (size_t i = 0; i < p.N; i++) {
        ret.push_back(sample_buffer[i+offset]);
    }
    return ret;
}

float mag(const std::complex<double> x) {
    return sqrtf(x.real() * x.real() + x.imag() * x.imag());
}

bool has_peak(const ProtocolConfig& p, const spectrum s, int i) {
    float m = mag(s[i]);
    float nm = mag(s[i+1]);
    float pm = mag(s[i-1]);
    return m > p.peak_threshold; // && m > nm && m > pm;
}

// detect begin of a message - should be 3 frames
//  - first must contain only f1
//  - second must contain only f2
//  - third must contain only f1
// adjust offset so that the amplitude of f2 in frames 1,3 should be as small as possible.
// Same for f1 in frame 2
//
// returns: offset in samples to apply for processing the message, or -1 if no begin marker discovered
int detect_begin(const ProtocolConfig& p){
    if (sample_buffer.size() < 5*p.N) return -1;
    std::vector<int> marker = {p.f2, p.f1, 0, p.f2};
    // float max_mag = 0;
    // float min_mag = 0;

    for(size_t i = 0; i < marker.size(); i++) {
        std::vector<float> frame = get_frame(p, i*p.N);
        spectrum s = get_spectrum(frame, p.sample_rate);
        if (i == 0){
            get_peaks(s, p.sample_rate, p.N);
        }
        if(marker[i] != 0 && !has_peak(p, s, marker[i])) {
            return -1;
        }
        std::cout << "F1: " << mag(s[p.f1]) << " F2: " << mag(s[p.f2]) << std::endl;
    }

    recorder_state = processing;
    // there are correct peaks, just need to align them correctly by applying offset
    std::cout << "Detected start marker" << std::endl;

    int best_offset_mag = 0;
    // int best_offset_phase = 0;

    float min_diff = 6;

    float max_mag = 0;

    std::fstream data;
    data.open("marker_data.csv", std::ios::out);
    data << "offset,F1,F2" << std::endl;
    for(int offset = 0; offset < p.N; offset++) {
        std::vector<float> frame = get_frame(p, p.N + offset);
        spectrum s = get_spectrum(frame, p.sample_rate);
        data << offset << "," << mag(s[p.f1]) << "," << mag(s[p.f2]) << std::endl;
        if (mag(s[p.f1]) > max_mag && offset < p.N) {
            max_mag = mag(s[p.f1]);
            best_offset_mag = offset;
        }
    }
    //     std::vector<float> frame2 = get_frame(p, p.N + offset);
    //     spectrum s2 = get_spectrum(frame2, p.sample_rate);
    //     float p1 = std::atan2(s[p.f1].imag(), s[p.f1].real());
    //     // frame = get_frame(p, 3*p.N + offset);
    //     // s = get_spectrum(frame, p.sample_rate);
    //     float p2 = std::atan2(s2[p.f2].imag(), s2[p.f2].real());
    //     if (std::abs(p1 - p2) < min_diff) {
    //         best_offset_phase = offset;
    //         min_diff = std::abs(p1-p2);
    //     }

    //     if (std::abs(mag(s[p.f2]) - mag(s[p.f1])) > max_mag - min_mag) {
    //         min_mag = mag(s[p.f1]);
    //         max_mag = mag(s[p.f2]);
    //         best_offset_mag = offset;
    //     }
    // }

    std::cout << "after correction mag: " << best_offset_mag << std::endl;
    for(int i = 0; i < marker.size(); i++) {
        std::vector<float> frame = get_frame(p, best_offset_mag + p.N * (i));
        spectrum s = get_spectrum(frame, p.sample_rate);
        std::cout << i << ". frame: F1:" << mag(s[p.f1]) << " - " << std::atan2(s[p.f1].imag(), s[p.f1].real());
        std::cout << " F2: " << mag(s[p.f2]) << " - " << std::atan2(s[p.f2].imag(), s[p.f2].real()) << std::endl;
        // get_peaks(s, p.sample_rate, p.N);
    }

    return best_offset_mag;
}


// enqueues sample to sample_buffer and removes old ones from it
void enqueue_frame(const std::vector<float>& samples, const ProtocolConfig& p) {
    assert(samples.size() == p.N);
    for(float s : samples) {
        sample_buffer.push_back(s);
    }

    while (sample_buffer.size() > 7*p.N) sample_buffer.pop_front();
    if (recorder_state == idle) {
        int x = detect_begin(p);
        if (x != -1) {
            sync_offset = x;
            recorder_state = processing;
        }
    } else if (recorder_state == processing) {
        if (msg_frames < 15) {
            auto frame = get_frame(p, 4*p.N + sync_offset);
            spectrum s = get_spectrum(frame, p.sample_rate);
            get_peaks(s, p.sample_rate, p.N);
            msg_frames ++;
        } else {
            recorder_state = idle;
            sample_buffer.clear();
        }
    }
}

// gets waveform for tx_bytes data
std::vector<float> modulate() {

}

// fills rx_bytes with new data
void demodulate(int offset, int frame) {

}
