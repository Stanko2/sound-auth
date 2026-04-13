#include "modulation.h"
#include "filter.h"
#include "waveforms.h"
#include <algorithm>
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
  std::cout << "lowest strength: " << p.lowest_strength << "dB threshold: " << p.strength_threshold << "dB\n";
  recorder_state = idle;
  waveforms = new Waveforms(10 * p.N, p.N, new GaussianWindow(p.N, 4.5));
  waveforms->addFilter((Filter *)new CombFilter(p.sample_rate, 200, 0));
  marker_file.open("test-data/marker", std::ios_base::app | std::ios::out);
  message_file.open("test-data/message", std::ios_base::app | std::ios::out);
  if (!message_file.is_open()) {
    std::cerr << "Failed to open message log file. Please create directory "
                 "'test-data'\n";
  }
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

  Spectrum* s0 = get_spectrum(1*p.N);
  Spectrum* s1 = get_spectrum(2*p.N);
  Spectrum* s2 = get_spectrum(3*p.N);

  if (s0->strength(p.f2) > p.strength_threshold) {
    return -1;
  }

  if (s1->strength(p.f2) < p.strength_threshold) {
    return -1;
  }

  if (s1->strength(p.f1) > p.strength_threshold) {
    return -1;
  }

  if (s2->strength(p.f1) < p.strength_threshold) {
    return -1;
  }

  delete s0, delete s1, delete s2;

  recorder_state = processing;
  // there are correct peaks, just need to align them correctly by applying
  // offset

  int best_offset_noise = 0;
  float min_noise_noise = 1e30;
  int best_offset_mag = 0;
  float max_mag = 0;
  float max_df1 = 0;
  int offset_max_df1 = 0;
  float max_df2 = 0;
  int offset_max_df2 = 0;

  std::fstream data;
  data.open("marker_data.csv", std::ios::out);
  data << "offset,F1,F2,noise,df1,df2" << std::endl;
  std::cout << "Detected start marker" << std::endl;
  Spectrum *s = nullptr;
  Spectrum *last = nullptr;
  for (int offset = 0; offset < 2 * p.N; offset++) {
    s = get_spectrum(p.N + offset);
    float noise = get_noise(s);
    data << offset << "," << s->strength(p.f1) << "," << s->strength(p.f2) << "," << noise << ",";
    if (last != nullptr) {
      float df1 = (s->strength(p.f1) - last->strength(p.f1)) * p.sample_rate;
      float df2 = (s->strength(p.f2) - last->strength(p.f2)) * p.sample_rate;
      data << df1 << "," << df2 << std::endl;
      if (df1 > max_df1) {
        max_df1 = df1;
        offset_max_df1 = offset;
      }

      if (df2 > max_df2) {
        max_df2 = df2;
        offset_max_df2 = offset;
      }
    } else {
      data << "0,0" << std::endl;
    }

    if (s->mag(p.f1) > max_mag) {
      max_mag = s->mag(p.f1);
      best_offset_mag = offset;
    }
    delete last;
    last = s;
  }
  delete s;

  float last_mag_f1 = 0;
  float last_mag_f2 = 0;
  int offset_derivative = (offset_max_df1 + offset_max_df2) / 2;
  std::cout << "after correction mag: " << best_offset_mag << std::endl;
  for (int i = 0; i < marker.size(); i++) {
    Spectrum* s = get_spectrum(best_offset_mag + p.N * (i - 2));
    std::cout << i << ". frame: F1:" << s->mag(p.f1) << " - " << s->phase(p.f1);
    std::cout << " F2: " << s->mag(p.f2) << " - " << s->phase(p.f2)
              << std::endl;
  }
  std::cout << "offset F1: " << offset_max_df1 << " offset F2: " << offset_max_df2 << " calculated offset: " << offset_derivative << std::endl;
  for (int i = 0; i < marker.size(); i++) {
    Spectrum* s = get_spectrum(offset_derivative + p.N * (i-1));
    std::cout << i << ". frame: F1:" << s->mag(p.f1) << " - " << s->phase(p.f1);
    std::cout << " F2: " << s->mag(p.f2) << " - " << s->phase(p.f2)
              << std::endl;
  }

  if (abs((offset_max_df1 - offset_max_df2) - 1000) < 150) {
    std::cout << "using max derivative synchronization" << std::endl;
    return offset_derivative;
  } else {
    std::cout << "using max magnitude synchronization" << std::endl;
    return best_offset_mag - p.N;
  }
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
    if (msg_frames < MAX_MESSAGE_SIZE) {
      demodulate();
      msg_frames++;
    } else {
      recorder_state = idle;
      message_data_file.close();
      msg_frames = 0;
      std::cout << std::endl;
      received_bytes.clear();
      if (rx_callback != nullptr) {
        rx_callback(received_bytes);
      }
    }
  }
}


float bin_to_freq(ProtocolConfig* p, int bin) {
  float delta = (float)p->sample_rate / (float)p->N;
  return bin * delta;
}


waveform SignalModulation::transmit_data(std::vector<uint8_t> &data) {
  assert(data.size() <= MAX_MESSAGE_SIZE);

  tx_buffer.clear();
  for (uint8_t byte : data) {
    for (int i = 7; i >= 0; i--) {
      tx_buffer.push_back((byte & (1 << i)) > 0);
    }
  }

  // TODO: add marker
  std::string f1 = std::to_string(bin_to_freq(&p, p.f1));
  std::string f2 = std::to_string(bin_to_freq(&p, p.f2));
  std::string marker = f2 + "|" + f1 + "|0|" + f2 + "|";

  assert(strategy != nullptr);

  std::string freq_string = marker + strategy->modulate(tx_buffer);
  std::cout << "MSG string: " << freq_string << std::endl;
  assert(tx_callback != nullptr);
  waveform w = waveforms->getWaveform(freq_string, p.N, p.sample_rate);

  tx_callback(w);
  return w;
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
  return waveforms->get_spectrum(frame, p.sample_rate, p.lowest_strength);
}

void SignalModulation::demodulate() {
  assert(strategy != nullptr);

  std::vector<bool> bits = strategy->demodulate(sync_offset + 4 * p.N);

  if (message_data_file.is_open()) {
    for (int i = 0; i <= p.N; i++) {
      waveform frame = waveforms->get_frame(sync_offset + 2 * p.N + i);
      Spectrum *s = waveforms->get_spectrum(frame, p.sample_rate, p.lowest_strength);
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
    received_bytes.push_back(ToByte(rx_buffer));
    rx_buffer.clear();
  }
}

void SignalModulation::set_strategy(ModulationStrategy *strategy) {
  this->strategy = strategy;
  strategy->Init(this, &p);
}

void SignalModulation::set_tx_callback(const std::function<void(waveform)> &cb) {
  tx_callback = cb;
}

void SignalModulation::set_rx_callback(const std::function<void(std::vector<uint8_t>)> &cb) {
  rx_callback = cb;
}

ProtocolConfig *createProtocolConfig(int N, int sample_rate, int f1, int f2,
                                     float min_strength, float strength_threshold) {
  ProtocolConfig *p = new ProtocolConfig();
  p->N = N;
  p->sample_rate = sample_rate;
  p->f1 = f1 * p->N / p->sample_rate;
  p->f2 = f2 * p->N / p->sample_rate;
  p->lowest_strength = min_strength;
  p->strength_threshold = strength_threshold;

  return p;
}

SignalModulation::~SignalModulation() {
  delete waveforms;
  marker_file.close();
  message_file.close();
}
