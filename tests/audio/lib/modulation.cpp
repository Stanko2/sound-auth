#include "modulation.h"
#include "filter.h"
#include "waveforms.h"
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <ios>
#include <iostream>
#include <mutex>
#include <ostream>
#include <string>
#include "entry.h"
#include <vector>

SignalModulation::SignalModulation(const ProtocolConfig &p) {
  this->p = p;
  recorder_state = idle;
  waveforms = new Waveforms(10 * p.N, p.N, new GaussianWindow(p.N, 4.5));
  waveforms->addFilter((Filter *)new CombFilter(p.sample_rate, 200, 0));

  if (SoundTransfer::LOG_LEVEL >= LogLevel::all) {
    marker_file.open("test-data/marker", std::ios_base::app | std::ios::out);
    message_file.open("test-data/message", std::ios_base::app | std::ios::out);
    if (!message_file.is_open()) {
      // std::cerr << "Failed to open message log file. Please create directory "
      //              "'test-data'\n";
    }

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
  const auto sync_begin = std::chrono::high_resolution_clock::now();
  Spectrum *s0 = get_spectrum(2 * p.N);
  Spectrum *s1 = get_spectrum(3 * p.N);
  Spectrum *s2 = get_spectrum(4 * p.N);

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
  if (SoundTransfer::LOG_LEVEL >= LogLevel::all) {
    data.open("marker_data.csv", std::ios::out);
    data << "offset,F1,F2,noise,df1,df2" << std::endl;
  }

  if (SoundTransfer::LOG_LEVEL >= LogLevel::warning) {
    std::cout << "Detected start marker" << std::endl;
  }

  Spectrum *s = nullptr;
  Spectrum *last = nullptr;
  const int offset_step = 5;
  for (int offset = 0; offset < 3 * p.N; offset += offset_step) {
    s = get_spectrum(p.N + offset);
    float noise = get_noise(s);
    if (SoundTransfer::LOG_LEVEL >= LogLevel::all) {
      data << offset << "," << s->strength(p.f1) << "," << s->strength(p.f2)
          << "," << noise << ",";
    }

    if (last != nullptr) {
      float df1 = (s->strength(p.f1) - last->strength(p.f1)) * p.sample_rate /
                  (float)offset_step;
      float df2 = (s->strength(p.f2) - last->strength(p.f2)) * p.sample_rate /
                  (float)offset_step;
      if (SoundTransfer::LOG_LEVEL >= LogLevel::all) {
        data << df1 << "," << df2 << std::endl;
      }

      if (df1 > max_df1) {
        max_df1 = df1;
        offset_max_df1 = offset;
      }

      if (df2 > max_df2) {
        max_df2 = df2;
        offset_max_df2 = offset;
      }
    } else {
      if(SoundTransfer::LOG_LEVEL >= LogLevel::all) {
        data << "0,0" << std::endl;
      }
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
  if (SoundTransfer::LOG_LEVEL >= LogLevel::info)  {
    std::cout << "after correction mag: " << best_offset_mag << std::endl;
  }
  for (int i = 0; i < marker.size(); i++) {
    Spectrum *s = get_spectrum(best_offset_mag + p.N * (i - 2));
    if (SoundTransfer::LOG_LEVEL >= LogLevel::info)  {
      std::cout << i << ". frame: F1:" << s->mag(p.f1) << " - " << s->phase(p.f1);
      std::cout << " F2: " << s->mag(p.f2) << " - " << s->phase(p.f2)
                << std::endl;
    }
    delete s;
  }

  if (SoundTransfer::LOG_LEVEL >= LogLevel::info)  {
    std::cout << "offset F1: " << offset_max_df1
              << " offset F2: " << offset_max_df2
              << " calculated offset: " << offset_derivative << std::endl;
  }
  for (int i = 0; i < marker.size(); i++) {
    Spectrum *s = get_spectrum(offset_derivative + p.N * (i - 1));
    if (SoundTransfer::LOG_LEVEL >= LogLevel::info)  {
      std::cout << i << ". frame: F1:" << s->mag(p.f1) << " - " << s->phase(p.f1);
      std::cout << " F2: " << s->mag(p.f2) << " - " << s->phase(p.f2)
                << std::endl;
    }
    delete s;
  }

  const auto sync_stop = std::chrono::high_resolution_clock::now();

  const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
      sync_stop - sync_begin);
  if (SoundTransfer::LOG_LEVEL >= LogLevel::warning)  {
    std::cout << "synchronization took: " << duration.count() << "ms"
              << std::endl;
  }

  data.close();
  if (abs((offset_max_df1 - offset_max_df2) - p.N) < 0.2 * p.N) {
    if (SoundTransfer::LOG_LEVEL >= LogLevel::info) {
      std::cout << "using max derivative synchronization" << std::endl;
    }
    return offset_derivative;
  } else {
    if (SoundTransfer::LOG_LEVEL >= LogLevel::warning) {
      std::cout << "using fallback max magnitude synchronization" << std::endl;
    }
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

  if (recorder_state == transmitting) {
    std::lock_guard<std::mutex> lock(state_mutex);
    msg_frames --;
    if (msg_frames == 0) {
      recorder_state = idle;
      state_cv.notify_all();
    }
  } else if (recorder_state == idle) {
    if (to_transmit.size() > 0) {
      Spectrum* s = get_spectrum(0);
      if (strategy != nullptr && !strategy->has_noise(s)) {
        tx_callback(to_transmit);
        msg_frames = to_transmit.size() / p.N + 1;
        to_transmit.clear();
        recorder_state = transmitting;
        return;
      }
      delete s;
    } else {
      int x = detect_begin();
      if (x != -1) {
        sync_offset = x;
        recorder_state = processing;
        msg_frames = 0;
      }
    }

  } else if (recorder_state == processing) {
    if (msg_frames < p.max_message_length) {
      demodulate();
      msg_frames++;
    } else {
      recorder_state = idle;
      message_data_file.close();
      msg_frames = 0;
      std::cout << std::endl;
      if (rx_callback != nullptr) {
        rx_callback(received_bytes);
      }
      received_bytes.clear();
      while (!rx_buffer.empty()) rx_buffer.pop();
    }
  }
}

float bin_to_freq(const ProtocolConfig *p, int bin) {
  float delta = (float)p->sample_rate / (float)p->N;
  return bin * delta;
}

void SignalModulation::transmit_data(std::vector<uint8_t> &data) {
  assert(data.size() <= p.max_message_length * strategy->bits_per_frame() / 8);

  std::vector<bool> tx_buffer;
  for (uint8_t byte : data) {
    for (int i = 7; i >= 0; i--) {
      tx_buffer.push_back((byte & (1 << i)) > 0);
    }
  }

  {
    std::unique_lock<std::mutex> lock(state_mutex);

    state_cv.wait(lock, [this](){
      return recorder_state == idle && to_transmit.empty();
    });

    std::string f1 = std::to_string(bin_to_freq(&p, p.f1));
    std::string f2 = std::to_string(bin_to_freq(&p, p.f2));
    std::string marker = f2 + "|" + f1 + "|0|" + f2 + "|";

    assert(strategy != nullptr);

    std::string freq_string = marker + strategy->modulate(tx_buffer);
    if (SoundTransfer::LOG_LEVEL >= LogLevel::info) {
      std::cout << "MSG string: " << freq_string << std::endl;
    }
    // assert(tx_callback != nullptr);
    to_transmit = waveforms->getWaveform(freq_string, p.N, p.sample_rate);

    state_cv.wait(lock, [this](){
      return recorder_state == idle && to_transmit.empty();
    });
  }
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
      Spectrum *s =
          waveforms->get_spectrum(frame, p.sample_rate, p.lowest_strength);
      message_data_file << s->strength(p.f1) << "," << s->strength(p.f2) << ","
                        << (i == p.N) << std::endl;

      delete s;
    }
  }

  for (auto i : bits) {
    rx_buffer.push(i);
  }

  while (rx_buffer.size() >= 8) {
    uint8_t byte = 0;
    for (int i = 0; i < 8; i++) {
      bool x = rx_buffer.front();
      rx_buffer.pop();
      if (x) {
        byte |= 1 << (7-i);
      }
    }

    std::cout << std::hex << (int) byte << std::dec << std::flush;
    received_bytes.push_back(byte);
  }
}

void SignalModulation::set_strategy(ModulationStrategy *strategy) {
  this->strategy = strategy;
  strategy->Init(this, &p);
}

void SignalModulation::set_tx_callback(
    const std::function<void(waveform)> &cb) {
  tx_callback = cb;
}

void SignalModulation::set_rx_callback(
    const std::function<void(std::vector<uint8_t>)> &cb) {
  rx_callback = cb;
}

ProtocolConfig *createProtocolConfig(int N, int sample_rate, int f1, int f2,
                                     float min_strength,
                                     float strength_threshold) {
  ProtocolConfig *p = new ProtocolConfig();
  p->N = N;
  p->sample_rate = sample_rate;
  p->f1 = f1 * p->N / p->sample_rate;
  p->f2 = f2 * p->N / p->sample_rate;
  p->lowest_strength = min_strength;
  p->strength_threshold = strength_threshold;
  p->max_message_length = 64;

  return p;
}

std::ostream &operator<<(std::ostream &os, const ProtocolConfig &config) {
  os << "ProtocolConfig {\n"
     << "  Marker Frequencies: " << bin_to_freq(&config, config.f1) << " Hz, " << bin_to_freq(&config, config.f2) << " Hz\n"
     << "  FFT Window Size (N): " << config.N << "\n"
     << "  Sample Rate: " << config.sample_rate << " Hz\n"
     << "  Strength: [Min: " << config.lowest_strength << "dB"
     << ", Threshold: " << config.strength_threshold << "dB]\n"
     << "  Max Message Length: " << config.max_message_length << " frames\n"
     << "}";
  return os;
}

SignalModulation::~SignalModulation() {
  delete waveforms;
  marker_file.close();
  message_file.close();
}
