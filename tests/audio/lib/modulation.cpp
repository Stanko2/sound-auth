#include "modulation.h"
#include "filter.h"
#include "waveforms.h"
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

SignalModulation::SignalModulation(const ProtocolConfig &p) {
  this->p = p;
  recorder_state = idle;
  waveforms = new Waveforms(6 * p.N, p.N, new GaussianWindow(p.N, 4.5));
  waveforms->addFilter((Filter *)new CombFilter(p.sample_rate, 200, 0));
  marker_file.open("test-data/marker", std::ios_base::app | std::ios::out);
  message_file.open("test-data/message", std::ios_base::app | std::ios::out);
  if (!message_file.is_open()) {
    std::cerr << "Failed to open message log file. Please create directory "
                 "'test-data'\n";
  }
}

float strength_level = 0;
float mag(const std::complex<double> x) {
  return sqrtf(x.real() * x.real() + x.imag() * x.imag());
}

bool SignalModulation::has_peak(Spectrum *s, int i) {
  float m = s->mag(i);
  float nm = s->mag(i + 1);
  float pm = s->mag(i - 1);
  return m > p.peak_threshold; // && m > nm && m > pm;
}

// detect begin of a message - should be 4 frames
//  - first must contain only f2
//  - second must contain only f1
//  - third is empty
//  - fourth must contain only f2
// adjust offset so that the amplitude of f1 in frame 2 is as large as possible
//
// returns: offset in samples to apply for processing the message, or -1 if no
// begin marker discovered
int SignalModulation::detect_begin() {
  // if (receive_sample_buffer->size() < 6*p.N) return -1;
  std::vector<int> marker = {p.f2, p.f1, 0, p.f2};
  // float max_mag = 0;
  // float min_mag = 0;

  for (size_t i = 1; i < marker.size(); i++) {
    Spectrum *s = get_spectrum(i * p.N);

    if (i == 1) {
      // waveforms->get_peaks(s, p.sample_rate, p.N);
    }
    if (marker[i - 1] != 0 && !has_peak(s, marker[i - 1])) {
      return -1;
    }
    std::cout << "F1: " << s->mag(p.f1) << " F2: " << s->mag(p.f2) << std::endl;
  }

  recorder_state = processing;
  // there are correct peaks, just need to align them correctly by applying
  // offset

  int best_offset_noise = 0;
  float min_noise_noise = 1e30;
  int best_offset_mag = 0;
  float max_mag = 0;

  std::fstream data;
  data.open("marker_data.csv", std::ios::out);
  data << "offset,F1,F2,noise" << std::endl;
  std::cout << "Detected start marker" << std::endl;
  for (int offset = 0; offset < 2 * p.N; offset++) {
    std::vector<float> frame = waveforms->get_frame(p.N + offset);
    Spectrum *s = waveforms->get_spectrum(frame, p.sample_rate);
    float noise = get_noise(s);
    data << offset << "," << s->mag(p.f1) << "," << s->mag(p.f2) << "," << noise
         << std::endl;

    if (s->mag(p.f1) > max_mag) {
      max_mag = s->mag(p.f1);
      best_offset_mag = offset;
    }
    delete s;
  }

  float last_mag_f1 = 0;
  float last_mag_f2 = 0;
  std::cout << "after correction mag: " << best_offset_mag << std::endl;
  for (int i = 0; i < marker.size(); i++) {
    std::vector<float> frame = waveforms->get_frame(best_offset_mag + p.N * i);
    Spectrum *s = waveforms->get_spectrum(frame, p.sample_rate);
    std::cout << i << ". frame: F1:" << s->mag(p.f1) << " - " << s->phase(p.f1);
    std::cout << " F2: " << s->mag(p.f2) << " - " << s->phase(p.f2)
              << std::endl;

    if (i == 0) {
      last_mag_f2 = s->mag(p.f2);
    }

    float thresholdCoef = 0.1f;
    if (i == 1) {
      float delta = last_mag_f2 - s->strength(p.f2);
      last_mag_f1 = s->strength(p.f1);
    }
    if (i == 2) {
      float delta = last_mag_f1 - s->strength(p.f1);
    }
  }

  return best_offset_mag;
}

float SignalModulation::get_noise(Spectrum *s) {
  float total_noise = 0;
  int count = 0;
  for (int i = 0; i < p.N / 2; i++) {
    if (abs(i - p.f1) < 10 || abs(i - p.f2) < 10)
      continue;
    total_noise += s->mag(i);
    count++;
  }

  return total_noise;
}

// enqueues sample to sample_buffer and removes old ones from it
void SignalModulation::enqueue_frame(const std::vector<float> &samples) {
  assert(samples.size() == p.N);
  waveforms->enqueue_frame(samples);
  // std::cout << "enqueue" << std::endl;
  if (recorder_state == idle) {
    int x = detect_begin();
    if (x != -1) {
      sync_offset = x;
      recorder_state = processing;
      msg_frames = 0;
      if (false) { // TODO: pridat switch na logovanie.
        int id = rand();
        message_data_file.open("test-data/message" + std::to_string(id));
        message_data_file << "f1,f2,reading" << std::endl;
      }
    }
  } else if (recorder_state == processing) {
    if (msg_frames < 64) {
      demodulate();
      msg_frames++;
    } else {
      recorder_state = idle;
      message_data_file.close();
      msg_frames = 0;
      strength_level = 0;
      std::cout << std::endl;
    }
  }
}

waveform SignalModulation::transmit_data(std::vector<uint8_t> &data) {
  tx_buffer.clear();
  for (uint8_t byte : data) {
    for (int i = 7; i >= 0; i--) {
      tx_buffer.push_back((byte & (1 << i)) > 0);
    }
  }

  assert(strategy != nullptr);

  std::string freq_string = strategy->modulate(tx_buffer);
  return waveforms->getWaveform(freq_string, p.N, p.sample_rate);
}

unsigned char ToByte(std::vector<bool> b) {
  unsigned char c = 0;
  for (int i = 0; i < 8; ++i)
    if (b[i])
      c |= 1 << (7 - i);
  return c;
}

Spectrum *SignalModulation::get_spectrum(int offset) {
  waveform frame = waveforms->get_frame(offset);
  return waveforms->get_spectrum(frame, p.sample_rate);
}

void SignalModulation::demodulate() {
  assert(strategy != nullptr);

  std::vector<bool> bits = strategy->demodulate(sync_offset + 3 * p.N);

  if (message_data_file.is_open()) {
    for (int i = 0; i <= p.N; i++) {
      waveform frame = waveforms->get_frame(sync_offset + 2 * p.N + i);
      Spectrum *s = waveforms->get_spectrum(frame, p.sample_rate);
      message_data_file << s->strength(p.f1) << "," << s->strength(p.f2) << ","
                        << (i == p.N) << std::endl;

      delete s;
    }
  }

  for (auto i : bits) {
    rx_buffer.push_back(i);
  }
  // std::cout << "Got: " << rx_buffer[rx_buffer.size() - 1] << std::endl;
  // std::cout << "Got: " << rx_buffer[rx_buffer.size() - 2] << std::endl;

  if (rx_buffer.size() == 8) {
    // message_file << "Received byte: " << ToByte(rx_buffer) << std::endl;
    std::cout << ToByte(rx_buffer);
    rx_buffer.clear();
  }
}

void SignalModulation::set_strategy(ModulationStrategy *strategy) {
  this->strategy = strategy;
  strategy->Init(this, &p);
}

ProtocolConfig *createProtocolConfig(int N, int sample_rate, int f1, int f2,
                                     float peak_threshold) {
  ProtocolConfig *p = new ProtocolConfig();
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
