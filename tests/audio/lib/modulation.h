#pragma once

#include <complex>
#include <cstdint>
#include <deque>
#include <fstream>
#include <vector>

#include "RingBuffer.h"
#include "waveforms.h"
#include "ModulationStrategies/modulationStrategy.h"

struct ProtocolConfig {
    // marker frequency 1
    int f1;
    // marker frequency 2
    int f2;
    // FFT window size / samples-per-frame
    int N;
    int sample_rate;
    float peak_threshold;
    // message length in frames
    size_t max_message_length;
};

enum State {
    idle = 0,
    processing = 1,
};

// Shared runtime buffers/state used by modulation/demodulation

class SignalModulation {
private:
  std::vector<bool> rx_buffer;
  std::vector<bool> tx_buffer;
  State recorder_state;
  ModulationStrategy* strategy = nullptr;
  int sync_offset;
  int msg_frames;
  ProtocolConfig p;
  Waveforms *waveforms;

  std::ofstream marker_file;
  std::ofstream message_file;
  std::ofstream message_data_file;

  /**
   * Check whether spectrum `s` has a peak at bin `i` according to protocol `p`.
   */
  bool has_peak(Spectrum* s, int i);


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
  float get_noise(Spectrum* s);
public:
  SignalModulation(const ProtocolConfig& p);

  void set_strategy(ModulationStrategy* strategy);

  Spectrum* get_spectrum(int offset);

  /**
   * Enqueue a frame of samples (length p.N) into the internal sample buffer and
   * trim old samples. May trigger detection or state transitions.
   */
  void enqueue_frame(const std::vector<float> &samples);

  /**
   * Build a transmit waveform for the contents of `tx_buffer`.
   * Returns the waveform samples to play.
   */
  waveform transmit_data(std::vector<uint8_t> &data);

  /**
   * Process/demodulate received data.
   * calculate spectrum at sync_offset and add coresponding bytes to rx_bytes
   */
  void demodulate();

  ~SignalModulation();
};

ProtocolConfig* createProtocolConfig(int N = 2048, int sample_rate = 48000,
                                  int f1 = 15000, int f2 = 17000,
                                  float peak_threshold = 10);
