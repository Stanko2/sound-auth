#pragma once

#include <complex>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <fstream>
#include <functional>
#include <mutex>
#include <queue>
#include <vector>

#include "ModulationStrategies/modulationStrategy.h"
#include "RingBuffer.h"
#include "waveforms.h"

struct ProtocolConfig {
  // marker frequency 1
  int f1;
  // marker frequency 2
  int f2;
  // FFT window size / samples-per-frame
  int N;
  int sample_rate;
  float lowest_strength;
  float strength_threshold;
  // message length in frames
  size_t max_message_length;

  int output_buffer_size = 100;
  int input_buffer_size = 30;
};

enum State {
  idle = 0,
  processing = 1,
  transmitting = 2,
};

std::ostream &operator<<(std::ostream &os, const ProtocolConfig &config);

// Shared runtime buffers/state used by modulation/demodulation

class SignalModulation {
private:
  State recorder_state;
  int sync_offset;
  int msg_frames;
  waveform to_transmit;

  ProtocolConfig p;
  Waveforms *waveforms;
  ModulationStrategy *strategy = nullptr;

  std::queue<bool> rx_buffer;
  std::vector<uint8_t> received_bytes;

  /*
   * function to call when message is received
   */
  std::function<void(std::vector<uint8_t> msg)> rx_callback;

  /*
   * Function to call to play some media
   */
  std::function<void(waveform waveform)> tx_callback;

  std::ofstream marker_file;
  std::ofstream message_file;
  std::ofstream message_data_file;

  std::mutex state_mutex;
  std::condition_variable state_cv;

  /**
   * Check whether spectrum `s` has a peak at bin `i` according to protocol `p`.
   */
  bool has_peak(Spectrum *s, int i);

  /*
   * Check if frequency f is present in the spectrum starting at frame_offset
   */
  bool is_present(int frame_offset, int f);

  /**
   * Detect the beginning (sync marker) of a message in the current
   * sample_buffer. Returns the best offset to align processing, or -1 if no
   * valid begin marker was found.
   */
  int detect_begin();

  /*
   * Calculates how much noise is in spectrum
   * (used to find correct synchronization offset)
   */
  float get_noise(Spectrum *s);

public:
  SignalModulation(const ProtocolConfig &p);

  void set_strategy(ModulationStrategy *strategy);

  Spectrum *get_spectrum(int offset);

  /**
   * Enqueue a frame of samples (length p.N) into the internal sample buffer and
   * removes old samples.
   */
  void enqueue_frame(const std::vector<float> &samples);

  /**
   * Build a transmit waveform for the contents of `tx_buffer`.
   * Returns the waveform samples to play.
   */
  void transmit_data(std::vector<uint8_t> &data);

  /**
   * Process/demodulate received data.
   * calculate spectrum at sync_offset and add coresponding bits to rx_buffer
   */
  void demodulate();

  // Set the callback that will be invoked when a waveform is ready to be
  // transmitted. The callback receives the generated waveform (vector<float>).
  void set_tx_callback(const std::function<void(waveform)> &cb);

  // Set the callback that will be invoked when a full message (bytes) is
  // received.
  void set_rx_callback(const std::function<void(std::vector<uint8_t>)> &cb);

  friend std::ostream& operator<<(std::ostream& os, const SignalModulation& sm) {
    os << sm.p << "\n" << *sm.strategy << '\n';
    return os;
  }

  ~SignalModulation();
};

/*
 * Create a ProtocolConfig structure
 * N: FFT window size
 * sample_rate: audio sample rate in Hz
 * f1: 1st marker frequency (in Hz)
 * f2: 2nd marker frequency (in Hz)
 * min_strength: intensity at which there is no signal - lowest possible
 * strength (in dB) strength_threshold: intensity at which there is considered
 * frequency as present (in dB)
 */
ProtocolConfig *createProtocolConfig(int N = 2048, int sample_rate = 48000,
                                     int f1 = 15000, int f2 = 17000,
                                     float min_strength = -100,
                                     float strength_threshold = -45);
