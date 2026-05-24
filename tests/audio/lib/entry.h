#pragma once
#include "ModulationStrategies/modulationStrategy.h"
#include "RingBuffer.h"
#include "modulation.h"
#include <atomic>
#include <cstdint>
#include <queue>
#include <thread>
#include <vector>

enum LogLevel {
  all = 3,
  info = 2,
  warning = 1,
  error = 0
};

class SoundTransfer {
private:
  SignalModulation* sm;
  ProtocolConfig* p;
  ModulationStrategy* strategy;
  int timeout = 0;
  Ringbuffer<float>* input_buffer;
  Ringbuffer<float>* output_buffer;
  std::atomic<bool> is_running;
  std::thread transfer_thread;
  std::vector<uint8_t> current_message;

  std::mutex recv_mutex;
  std::condition_variable recv_cv;
  std::queue<uint8_t> data_buffer;

public:
  static LogLevel LOG_LEVEL;
  SoundTransfer(ModulationStrategy* strategy, ProtocolConfig* p);

  Ringbuffer<float>* get_input_buffer() {
    return input_buffer;
  }
  Ringbuffer<float>* get_output_buffer() {
    return output_buffer;
  }

  void run();
  void stop();
  void send(std::vector<uint8_t> msg);

  void set_timeout(int timeout);

  size_t get_chunk_size();
  std::vector<uint8_t> recv(size_t length, bool clear = true);
  ~SoundTransfer();
};
