#include "entry.h"
#include "ModulationStrategies/modulationStrategy.h"
#include "RingBuffer.h"
#include "modulation.h"
#include <cstdint>
#include <mutex>
#include <thread>
#include <vector>

SoundTransfer::SoundTransfer(ModulationStrategy* strategy, ProtocolConfig* p) {
  this->p = p;
  sm = new SignalModulation(*p);
  sm->set_strategy(strategy);
  is_running = false;
  output_buffer = new Ringbuffer<float>(p->output_buffer_size * p->N);
  input_buffer = new Ringbuffer<float>(p->input_buffer_size * p->N);
  sm->set_tx_callback([&](waveform w) {
    output_buffer->resize(w.size());
    for (float a : w)
      output_buffer->add(a);
  });
  sm->set_rx_callback([&](std::vector<uint8_t> msg) {
    std::lock_guard<std::mutex> lock(recv_mutex);
    current_message.clear();
    for (uint8_t x : msg){
      current_message.push_back(x);
    }

    recv_cv.notify_all();
  });
}

void SoundTransfer::run() {
  is_running = true;
  transfer_thread = std::thread([this](){
    std::cout << "running" << std::endl;
    std::vector<float> frame(p->N);
    while (is_running) {
      while (input_buffer->size() < p->N) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
      }
      if (!is_running)
        break;
      for (int i = 0; i < p->N; i++) {
        input_buffer->pop(frame[i]);
      }
      sm->enqueue_frame(frame);
    }
  });

  // worker.join();
  // return worker;
}

void SoundTransfer::stop() {
  is_running = false;
}

void SoundTransfer::send(std::vector<uint8_t> msg) {
  // TODO: split message to smaller chunk, apply padding if short
  if (msg.size() >= p->max_message_length) {
    std::cerr << "Message too long" << std::endl;
    return;
  }


  sm->transmit_data(msg);
}

std::vector<uint8_t> SoundTransfer::recv() {
  std::unique_lock<std::mutex> lock(recv_mutex);
  current_message.clear();
  recv_cv.wait(lock, [this]() {
    return current_message.size() > 0;
  });

  return std::vector<uint8_t>(current_message);
}

SoundTransfer::~SoundTransfer() {
  delete sm;
  delete input_buffer;
  delete output_buffer;
}
