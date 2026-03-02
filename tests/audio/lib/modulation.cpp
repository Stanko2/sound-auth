#include "modulation.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <complex>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <fstream>
#include <ios>
#include <iostream>
#include <iterator>
#include <sstream>
#include <vector>
#include "RingBuffer.h"
#include "waveforms.h"
#include "filter.h"

SignalModulation::SignalModulation(const ProtocolConfig& p) {
    this->p = p;
    recorder_state = idle;
    waveforms = new Waveforms(7*p.N, p.N);
    thresholds.resize(p.N, 0);
    waveforms->addFilter((Filter*) new CombFilter(p.sample_rate, 20, 0.1f));
}

float mag(const std::complex<double> x) {
    return sqrtf(x.real() * x.real() + x.imag() * x.imag());
}

bool SignalModulation::has_peak(Spectrum* s, int i) {
    float m = s->mag(i);
    float nm = s->mag(i+1);
    float pm = s->mag(i-1);
    return m > p.peak_threshold && m > nm && m > pm;
}

// detect begin of a message - should be 3 frames
//  - first must contain only f1
//  - second must contain only f2
//  - third must contain only f1
// adjust offset so that the amplitude of f2 in frames 1,3 should be as small as possible.
// Same for f1 in frame 2
//
// returns: offset in samples to apply for processing the message, or -1 if no begin marker discovered
int SignalModulation::detect_begin(){
    // if (receive_sample_buffer->size() < 6*p.N) return -1;
    std::vector<int> marker = {p.f2, p.f1, 0, p.f2};
    // float max_mag = 0;
    // float min_mag = 0;

    for(size_t i = 1; i < marker.size(); i++) {
        std::vector<float> frame = waveforms->get_frame(i*p.N);
        Spectrum* s = waveforms->get_spectrum(frame, p.sample_rate);
        if (i == 1){
            // waveforms->get_peaks(s, p.sample_rate, p.N);
        }
        if(marker[i - 1] != 0 && !has_peak(s, marker[i - 1])) {
            return -1;
        }
        std::cout << "F1: " << s->mag(p.f1) << " F2: " << s->mag(p.f2) << std::endl;
    }

    recorder_state = processing;
    // there are correct peaks, just need to align them correctly by applying offset

    int best_offset_noise = 0;
    float min_noise_noise = 1e30;
    int best_offset_mag = 0;
    float max_mag = 0;

    std::fstream data;
    // waveform w(std::make_move_iterator(receive_sample_buffer.begin()), std::make_move_iterator(receive_sample_buffer.end()));
    // waveforms->saveToWav("marker.wav", w, p.sample_rate);
    data.open("marker_data.csv", std::ios::out);
    data << "offset,F1,F2,noise" << std::endl;
    std::cout << "Detected start marker" << std::endl;
    for(int offset = 0; offset < p.N; offset++) {
        // assert(2*p.N + offset < receive_sample_buffer->size());
        std::vector<float> frame = waveforms->get_frame(p.N + offset);
        Spectrum* s = waveforms->get_spectrum(frame, p.sample_rate);
        float noise = get_noise(s);
        data << offset << "," << s->mag(p.f1) << "," << s->mag(p.f2) << "," << noise << std::endl;
        if (noise < min_noise_noise) {
            min_noise_noise = noise;
            best_offset_noise = offset;
        }

        if (s->mag(p.f1) > max_mag) {
            max_mag = s->mag(p.f1);
            best_offset_mag = offset;
        }
        delete s;
    }

    std::cout << "after correction noise: " << best_offset_noise << std::endl;
    for(int i = 0; i < marker.size(); i++) {
        std::vector<float> frame = waveforms->get_frame(best_offset_noise + p.N * (i+1));
        Spectrum* s = waveforms->get_spectrum(frame, p.sample_rate);
        std::cout << i << ". frame: F1:" << s->mag(p.f1) << " - " << s->phase(p.f1);
        std::cout << " F2: " << s->mag(p.f2) << " - " << s->phase(p.f2) << std::endl;
    }

    float last_mag_f1 = 0;
    float last_mag_f2 = 0;
    std::cout << "after correction mag: " << best_offset_mag << std::endl;
    for(int i = 0; i < marker.size(); i++) {
        std::vector<float> frame = waveforms->get_frame(best_offset_mag + p.N * i);
        Spectrum* s = waveforms->get_spectrum(frame, p.sample_rate);
        std::cout << i << ". frame: F1:" << s->mag(p.f1) << " - " << s->phase(p.f1);
        std::cout << " F2: " << s->mag(p.f2) << " - " << s->phase(p.f2) << std::endl;

        if (i == 0) {
            last_mag_f2 = s->mag(p.f2);
        }
        if (i == 1) {
            thresholds[p.f2] = (s->mag(p.f2) + last_mag_f2) / 2;
            last_mag_f1 = s->mag(p.f1);
        }
        if (i == 2) {
            thresholds[p.f1] = (s->mag(p.f1) + last_mag_f1) / 2;
        }
    }


    return best_offset_mag;
}

float SignalModulation::get_noise(Spectrum* s) {
    float total_noise_db = 0;
    int count = 0;
    for(int i = 0; i < p.N / 2; i++) {
        if (abs(i - p.f1) < 10 || abs(i - p.f2) < 10) continue;

        float raw_mag = s->mag(i);
        float normalized_mag = raw_mag / p.N;

        float mag_db = 20.0f * log10(std::max(normalized_mag, 1e-7f));

        total_noise_db += mag_db;
        count++;
    }

    return (count > 0) ? (total_noise_db / count) : -140.0f;
}

// enqueues sample to sample_buffer and removes old ones from it
void SignalModulation::enqueue_frame(const std::vector<float>& samples) {
    assert(samples.size() == p.N);
    waveforms->enqueue_frame(samples);

    if (recorder_state == idle) {
        int x = detect_begin();
        if (x != -1) {
            sync_offset = x;
            recorder_state = processing;
        }
    } else if (recorder_state == processing) {
        if (msg_frames < 30) {
            demodulate();
        } else {
            recorder_state = idle;
            msg_frames = 0;
            // receive_sample_buffer->clear();
        }
    }
}

std::vector<float> SignalModulation::modulate() {
    std::string frequencies[2] =  {"15000", "17000"};
    std::string waveformString = "17000|15000|0|17000|";

    for (int i = 0; i < tx_buffer.size(); i+= 2) {
        bool added = false;
        for (int j = 0; i < 2; j ++) {
            if (tx_buffer[i+j]) {
                waveformString += frequencies[j] + ",";
            }
        }
        if (!added) {
            waveformString += "0|";
        }
        waveformString += "|";
    }

    std::cout << "Waveform string: " << waveformString << std::endl;
    return waveforms->getWaveform(waveformString, p.N, p.sample_rate);
}

waveform SignalModulation::transmit_data(std::vector<uint8_t> &data) {
    tx_buffer.clear();
    for (uint8_t byte: data)  {
        for (int i = 0; i < sizeof(uint8_t); i ++) {
            tx_buffer.push_back((byte & (1 << i)) > 0);
        }
    }

    return modulate();
}

unsigned char ToByte(std::vector<bool> b)
{
    unsigned char c = 0;
    for (int i=0; i < 8; ++i)
        if (b[i])
            c |= 1 << i;
    return c;
}

void SignalModulation::demodulate() {
    waveform frame = waveforms->get_frame(sync_offset);
    Spectrum* s = waveforms->get_spectrum(frame, p.sample_rate);
    rx_buffer.push_back(s->mag(p.f1) >= thresholds[p.f1]);
    rx_buffer.push_back(s->mag(p.f2) >= thresholds[p.f2]);

    if (rx_buffer.size() == 8) {
        std::cout << ToByte(rx_buffer);
        rx_buffer.clear();
    }
}

ProtocolConfig* createProtocolConfig(int N, int sample_rate, int f1, int f2, float peak_threshold) {
    ProtocolConfig* p = new ProtocolConfig();
    p->N = N;
    p->sample_rate = sample_rate;
    p->peak_threshold = peak_threshold;
    p->f1 = f1 * p->N / p->sample_rate;
    p->f2 = f2 * p->N / p->sample_rate;

    return p;
}
