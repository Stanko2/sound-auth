#pragma once
#include "ModulationStrategies/modulationStrategy.h"
#include "RingBuffer.h"
#include "modulation.h"
#include <atomic>
#include <cstdint>
#include <thread>

class SoundTransfer {
private:
  SignalModulation* sm;
  ProtocolConfig* p;
  Ringbuffer<float>* input_buffer;
  Ringbuffer<float>* output_buffer;
  std::atomic<bool> is_running;
  std::thread transfer_thread;
  std::vector<uint8_t> current_message;

  std::mutex recv_mutex;
  std::condition_variable recv_cv;

public:
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
  std::vector<uint8_t> recv();
  ~SoundTransfer();
};
