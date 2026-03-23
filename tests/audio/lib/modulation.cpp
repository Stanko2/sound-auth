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
  thresholds.resize(p.N, 0);
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

// detect begin of a message - should be 3 frames
//  - first must contain only f1
//  - second must contain only f2
//  - third must contain only f1
// adjust offset so that the amplitude of f2 in frames 1,3 should be as small as
// possible. Same for f1 in frame 2
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
  // waveform w(std::make_move_iterator(receive_sample_buffer.begin()),
  // std::make_move_iterator(receive_sample_buffer.end()));
  // waveforms->saveToWav("marker.wav", w, p.sample_rate);
  data.open("marker_data.csv", std::ios::out);
  data << "offset,F1,F2,noise" << std::endl;
  std::cout << "Detected start marker" << std::endl;
  for (int offset = 0; offset < 2 * p.N; offset++) {
    // assert(2*p.N + offset < receive_sample_buffer->size());
    std::vector<float> frame = waveforms->get_frame(p.N + offset);
    Spectrum *s = waveforms->get_spectrum(frame, p.sample_rate);
    float noise = get_noise(s);
    data << offset << "," << s->mag(p.f1) << "," << s->mag(p.f2) << "," <<
    noise << std::endl;
    // if (noise < min_noise_noise) {
    //     min_noise_noise = noise;
    //     best_offset_noise = offset;
    // }

    if (s->mag(p.f1) > max_mag) {
      max_mag = s->mag(p.f1);
      best_offset_mag = offset;
    }
    delete s;
  }

  // marker_file << "after correction noise: " << best_offset_noise <<
  // std::endl; for(int i = 0; i < marker.size(); i++) {
  //     std::vector<float> frame = waveforms->get_frame(best_offset_noise + p.N
  //     * (i+1)); Spectrum* s = waveforms->get_spectrum(frame, p.sample_rate);
  //     marker_file << i << ". frame: F1:" << s->mag(p.f1) << " - " <<
  //     s->phase(p.f1); marker_file << " F2: " << s->mag(p.f2) << " - " <<
  //     s->phase(p.f2) << std::endl;
  // }

  float last_mag_f1 = 0;
  float last_mag_f2 = 0;
  std::cout << "after correction mag: " << best_offset_mag << std::endl;
  for (int i = 0; i < marker.size(); i++) {
    std::vector<float> frame = waveforms->get_frame(best_offset_mag + p.N * i);
    Spectrum *s = waveforms->get_spectrum(frame, p.sample_rate);
    std::cout << i << ". frame: F1:" << s->mag(p.f1) << " - "
              << s->phase(p.f1);
    std::cout << " F2: " << s->mag(p.f2) << " - " << s->phase(p.f2)
              << std::endl;

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
  message_file << "Using thresholds: F1:" << thresholds[p.f1]
               << "dB F2:" << thresholds[p.f2] << std::endl;
  if (thresholds[p.f1] > -20 || thresholds[p.f2] > -20) {
    return -1;
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
      thresholds[p.f1] = -40;
      thresholds[p.f2] = -35;
      if (false) { // TODO: pridat switch na logovanie.
        int id = rand();
        message_data_file.open("test-data/message" + std::to_string(id));
        message_data_file << "f1,f2,reading" << std::endl;
      }
      // std::cout << "Using threshold: F1: " << thresholds[p.f1] << " F2: " <<
      // thresholds[p.f2] << std::endl;
    }
  } else if (recorder_state == processing) {
    // std::cout << "processing" << std::endl;
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

float SignalModulation::bin_to_frequency(int bin) {
  float delta = (float)p.sample_rate / (float)p.N;
  return bin * delta;
}

std::vector<float> SignalModulation::modulate() {
  std::cout << "f1 bin: " << p.f1 << " f2 bin: " << p.f2 << std::endl;
  std::string f1_freq = std::to_string(bin_to_frequency(p.f1));
  std::string f2_freq = std::to_string(bin_to_frequency(p.f2));
  std::string f1_on_freq = std::to_string(bin_to_frequency(p.f1 + 10));
  std::string f2_on_freq = std::to_string(bin_to_frequency(p.f2 + 10));
  std::string frequencies[4] = {f1_freq, f2_freq, f1_on_freq, f2_on_freq};
  std::string waveformString = f2_freq + "|" + f1_freq + "|0|" + f2_freq + "|";
  std::cout << "Modulate " << tx_buffer.size() << std::endl;
  for (int i = 0; i < tx_buffer.size(); i += 2) {
    bool added = false;
    for (int j = 0; j < 2; j++) {
      if (tx_buffer[i + j]) {
        waveformString += frequencies[j + 2] + ",";
        added = true;
      } else {
        waveformString += frequencies[j] + ",";
        added = true;
      }
    }

    waveformString = waveformString.substr(0, waveformString.size() - 1);
    if (!added) {
      waveformString += "0|";
    } else {
      waveformString += "|";
    }
  }

  waveformString = waveformString.substr(0, waveformString.size() - 1);
  std::cout << "Waveform string: " << waveformString << std::endl;
  return waveforms->getWaveform(waveformString, p.N, p.sample_rate);
}

waveform SignalModulation::transmit_data(std::vector<uint8_t> &data) {
  tx_buffer.clear();
  for (uint8_t byte : data) {
    for (int i = 7; i >= 0; i--) {
      tx_buffer.push_back((byte & (1 << i)) > 0);
    }
  }

  return modulate();
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

bool SignalModulation::is_present(int frame_offset, int f) {
  // int start = frame_offset - 80;
  // int end = frame_offset + 80;
  // float local_max = -100, local_min = 0;
  // bool decaying = true;

  // float last_strength = 0;
  // float strength_start, strength_end;
  // for (int i = start; i <= end; i += 8) {
  //   Spectrum *s = get_spectrum(i);
  //   float strength = s->strength(f);
  //   if (i == start)
  //     strength_start = strength;
  //   if (i == end)
  //     strength_end = strength;

  //   if (i < frame_offset && last_strength < strength)
  //     decaying = false;

  //   if (strength > local_max)
  //     local_max = strength;

  //   if (strength < local_min)
  //     local_min = strength;

  //   last_strength = strength;
  //   delete s;
  // }

  // // ak sme pod thresholdom, frekvencia tam nie je
  // if (local_max <= thresholds[f]) {
  //   return false;
  // }

  // if (decaying && local_max - local_min > 2) {
  //   return false;
  // }

  // if (strength_start - strength_end > 3)
  //   return false;

  // if (strength_level != 0 && strength_level - local_max > 10)
  //   return false;

  // strength_level = local_max;
  // return true;
  //
  Spectrum* s = get_spectrum(frame_offset);

  return s->strength(f + 10) >= s->strength(f);
}

void SignalModulation::demodulate() {
  waveform frame = waveforms->get_frame(sync_offset + 3 * p.N);
  Spectrum *s = waveforms->get_spectrum(frame, p.sample_rate);
  message_file << "message frame #" << msg_frames;
  message_file << " F1: " << s->strength(p.f1) << " F2: " << s->strength(p.f2)
               << " ";
  bool bit1 = is_present(sync_offset + 3 * p.N, p.f1);
  bool bit2 = is_present(sync_offset + 3 * p.N, p.f2);
  message_file << " " << bit1 << bit2 << std::endl;
  // message_file << " noise: " << get_noise(s) << std::endl;
  if (message_data_file.is_open()) {
    for (int i = 0; i <= p.N; i++) {
      waveform frame = waveforms->get_frame(sync_offset + 2 * p.N + i);
      Spectrum *s = waveforms->get_spectrum(frame, p.sample_rate);
      message_data_file << s->strength(p.f1) << "," << s->strength(p.f2) << ","
                        << (i == p.N) << std::endl;

      delete s;
    }
  }

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
