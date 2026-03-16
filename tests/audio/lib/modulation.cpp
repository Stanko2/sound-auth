#include "modulation.h"
#include <cassert>
#include <cmath>
#include <complex>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <ios>
#include <iostream>
#include <string>
#include <vector>
#include "waveforms.h"
#include "filter.h"

SignalModulation::SignalModulation(const ProtocolConfig& p) {
    this->p = p;
    recorder_state = idle;
    waveforms = new Waveforms(6*p.N, p.N);
    thresholds.resize(p.N, 0);
    waveforms->addFilter((Filter*) new CombFilter(p.sample_rate, 20, 0));
    marker_file.open("test-data/marker", std::ios_base::app | std::ios::out);
    message_file.open("test-data/message", std::ios_base::app | std::ios::out);
    if (!message_file.is_open()) {
        std::cerr << "Failed to open message log file. Please create directory 'test-data'\n";
    }
}

float mag(const std::complex<double> x) {
    return sqrtf(x.real() * x.real() + x.imag() * x.imag());
}

bool SignalModulation::has_peak(Spectrum* s, int i) {
    float m = s->mag(i);
    float nm = s->mag(i+1);
    float pm = s->mag(i-1);
    return m > p.peak_threshold; //&& m > nm && m > pm;
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
    for(int offset = 0; offset < 3*p.N; offset++) {
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

    marker_file << "after correction noise: " << best_offset_noise << std::endl;
    for(int i = 0; i < marker.size(); i++) {
        std::vector<float> frame = waveforms->get_frame(best_offset_noise + p.N * (i+1));
        Spectrum* s = waveforms->get_spectrum(frame, p.sample_rate);
        marker_file << i << ". frame: F1:" << s->mag(p.f1) << " - " << s->phase(p.f1);
        marker_file << " F2: " << s->mag(p.f2) << " - " << s->phase(p.f2) << std::endl;
    }

    float last_mag_f1 = 0;
    float last_mag_f2 = 0;
    marker_file << "after correction mag: " << best_offset_mag << std::endl;
    for(int i = 0; i < marker.size(); i++) {
        std::vector<float> frame = waveforms->get_frame(best_offset_mag + p.N * i);
        Spectrum* s = waveforms->get_spectrum(frame, p.sample_rate);
        marker_file << i << ". frame: F1:" << s->mag(p.f1) << " - " << s->phase(p.f1);
        marker_file << " F2: " << s->mag(p.f2) << " - " << s->phase(p.f2) << std::endl;

        if (i == 0) {
            last_mag_f2 = s->mag(p.f2);
        }

        float thresholdCoef = 0.1f;
        if (i == 1) {
            float delta = last_mag_f2 - s->strength(p.f2);
            thresholds[p.f2] = s->strength(p.f2) + thresholdCoef * delta;
            last_mag_f1 = s->strength(p.f1);
        }
        if (i == 2) {
            float delta = last_mag_f1 - s->strength(p.f1);
            thresholds[p.f1] = s->strength(p.f1) + thresholdCoef * delta;
        }
    }
    message_file << "Using thresholds: F1:" << thresholds[p.f1] << "dB F2:" << thresholds[p.f2] << std::endl;
    if (thresholds[p.f1] > -20 || thresholds[p.f2] > -20) {
        return -1;
    }

    return best_offset_mag;
}

float SignalModulation::get_noise(Spectrum* s) {
    float total_noise = 0;
    int count = 0;
    for(int i = 0; i < p.N / 2; i++) {
        if (abs(i - p.f1) < 10 || abs(i - p.f2) < 10) continue;
        total_noise += s->mag(i);
        count++;
    }

    return total_noise;
}

// enqueues sample to sample_buffer and removes old ones from it
void SignalModulation::enqueue_frame(const std::vector<float>& samples) {
    assert(samples.size() == p.N);
    waveforms->enqueue_frame(samples);
    // std::cout << "enqueue" << std::endl;
    if (recorder_state == idle) {
        int x = detect_begin();
        if (x != -1) {
            sync_offset = x;
            recorder_state = processing;
            msg_frames = 0;
            thresholds[p.f1] = -40;
            thresholds[p.f2] = -40;
            // std::cout << "Using threshold: F1: " << thresholds[p.f1] << " F2: " << thresholds[p.f2] << std::endl;
        }
    } else if (recorder_state == processing) {
        // std::cout << "processing" << std::endl;
        if (msg_frames < 16) {
            demodulate();
            msg_frames ++;
        } else {
            recorder_state = idle;
            msg_frames = 0;
            std::cout << std::endl;
        }
    }
}

std::vector<float> SignalModulation::modulate() {
    std::string frequencies[2] =  {"15000", "17000"};
    std::string waveformString = "17000|15000|0|17000|";
    std::cout << "Modulate " << tx_buffer.size() << std::endl;
    for (int i = 0; i < tx_buffer.size(); i+= 2) {
        bool added = false;
        for (int j = 0; j < 2; j++) {
            if (tx_buffer[i+j]) {
                waveformString += frequencies[j] + ",";
                added = true;
            }
        }
        if (!added) {
            waveformString += "0|";
        } else {
            waveformString += "|";
        }
    }

    std::cout << "Waveform string: " << waveformString << std::endl;
    return waveforms->getWaveform(waveformString, p.N, p.sample_rate);
}

waveform SignalModulation::transmit_data(std::vector<uint8_t> &data) {
    tx_buffer.clear();
    for (uint8_t byte: data)  {
        for (int i = 7; i >= 0; i --) {
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
            c |= 1 << (7-i);
    return c;
}

Spectrum* SignalModulation::get_spectrum(int offset) {
    waveform frame = waveforms->get_frame(offset);
    return waveforms->get_spectrum(frame, p.sample_rate);
}

bool SignalModulation::is_present(int frame_offset, int f) {
    Spectrum* s = get_spectrum(frame_offset);
    float curr = s->strength(f);
    // ak sme pod thresholdom, frekvencia tam nie je
    if (curr <= thresholds[f]){
        delete s;
        return false;
    }

    delete s;
    Spectrum *s_next = get_spectrum(frame_offset + p.N / 6);
    Spectrum *s_last = get_spectrum(frame_offset - p.N / 6);

    // Sme nad thresholdom - musime sa pozriet na to, ci sa meni pomaly
    // Ak sa meni pomaly, tak mame pravdepodobne kopec -> frekvencia tam bude

    float next = s_next->strength(f);
    float last = s_last->strength(f);
    float delta = std::abs(next - last);


    // return last <= curr && curr >= next;
    if (next < thresholds[f] || last < thresholds[f]) {
        delete s_last;
        delete s_next;
        return false;
    }

    delete s_last;
    delete s_next;
    // message_file << " delta: " << delta;
    return delta <= 7;
}


void SignalModulation::demodulate() {
    waveform frame = waveforms->get_frame(sync_offset + 3*p.N);
    Spectrum* s = waveforms->get_spectrum(frame, p.sample_rate);
    message_file << "message frame #" << msg_frames;
    message_file << " F1: " << s->strength(p.f1) << " F2: " << s->strength(p.f2) << " ";
    bool bit1 = is_present(sync_offset + 3*p.N, p.f1);
    bool bit2 = is_present(sync_offset + 3*p.N, p.f2);
    message_file << " " << bit1 << bit2 << std::endl;
    // message_file << " noise: " << get_noise(s) << std::endl;

    // for (int i = 0; i <= p.N; i++) {
    //     waveform frame = waveforms->get_frame(sync_offset + 2*p.N + i);
    //     Spectrum* s = waveforms->get_spectrum(frame, p.sample_rate);
    //     message_file << s->strength(p.f1) << "," << s->strength(p.f2) << "," << (i == p.N) << std::endl;

    //     delete s;
    // }

    rx_buffer.push_back(bit1);
    rx_buffer.push_back(bit2);
    // std::cout << "Got: " << rx_buffer[rx_buffer.size() - 1] << std::endl;
    // std::cout << "Got: " << rx_buffer[rx_buffer.size() - 2] << std::endl;

    if (rx_buffer.size() == 8) {
        message_file << "Received byte: " << ToByte(rx_buffer) << std::endl;
        std::cout << ToByte(rx_buffer);
        rx_buffer.clear();
    }

    delete s;
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

SignalModulation::~SignalModulation() {
    delete waveforms;
    marker_file.close();
    message_file.close();
}
