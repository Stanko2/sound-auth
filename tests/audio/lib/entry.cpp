#include "entry.h"
#include "ModulationStrategies/modulationStrategy.h"
#include "RingBuffer.h"
#include "modulation.h"
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <mutex>
#include <thread>
#include <vector>

LogLevel SoundTransfer::LOG_LEVEL = LogLevel::warning;

SoundTransfer::SoundTransfer(ModulationStrategy *strategy, ProtocolConfig *p) {
  this->p = p;
  this->strategy = strategy;
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

    // std::cout << msg.size() << ' ';
    // std::cout << "Receive callback: ";

    for (uint8_t x : msg) {
      current_message.push_back(x);
      // std::cout << std::hex << std::setw(2) << (int)x;
    }

    // std::cout << std::endl;

    recv_cv.notify_all();
  });
}

void SoundTransfer::run() {
  is_running = true;
  transfer_thread = std::thread([this]() {
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
      // std::cout << "enqueue frame" << std::endl;
      sm->enqueue_frame(frame);
    }
  });

  // worker.join();
  // return worker;
}

void SoundTransfer::stop() { is_running = false; }

void SoundTransfer::send(std::vector<uint8_t> msg) {
  size_t chunk_len = strategy->bits_per_frame() * p->max_message_length;

  if (msg.size() < chunk_len / 8) {
    sm->transmit_data(msg);
    return;
  }

  if (chunk_len % 8 != 0) {
    std::cerr << "Current config does not support message partitioning"
              << std::endl;
    return;
  }

  size_t chunk_len_bytes = chunk_len / 8;
  size_t begin = 0;

  std::vector<uint8_t> to_send;
  while (begin < msg.size()) {
    size_t size = std::min(chunk_len_bytes, msg.size() - begin);
    to_send.resize(size);
    memcpy(to_send.data(), msg.data() + begin, size);
    sm->transmit_data(to_send);

    begin += chunk_len_bytes;
  }
}

uint64_t timeSinceEpochMillisec() {
  using namespace std::chrono;
  return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

void SoundTransfer::set_timeout(int timeout) {
  this->timeout = timeout;
}

std::vector<uint8_t> SoundTransfer::recv(size_t length, bool clear) {
  std::vector<uint8_t> ret;
  ret.reserve(length);
  int received = 0;

  if (clear) {
    while (!data_buffer.empty())
      data_buffer.pop();
  }

  while (!data_buffer.empty() && ret.size() < length) {
    ret.push_back(data_buffer.front());
    data_buffer.pop();
  }

  size_t chunk_len = strategy->bits_per_frame() * p->max_message_length;
  size_t chunk_len_bytes = chunk_len / 8;

  if (length > chunk_len_bytes && chunk_len % 8 != 0) {
    std::cerr << "Current config does not support message partitioning"
              << std::endl;
    ret.resize(0);
    return ret;
  }

  while (ret.size() < length) {
    std::unique_lock<std::mutex> lock(recv_mutex);

    if (timeout == 0) {
      recv_cv.wait(lock, [this]() {
        return !current_message.empty();
      });
    } else {
      recv_cv.wait_for(lock, std::chrono::milliseconds(timeout),  [this]() {
        return !current_message.empty();
      });
    }

    if (current_message.empty() && clear) {
      return ret;
    }

    size_t needed = length - ret.size();
    size_t available = current_message.size();
    size_t to_copy = std::min(needed, available);

    ret.insert(ret.end(), current_message.begin(),
               current_message.begin() + to_copy);

    for (size_t i = to_copy; i < available; ++i) {
      data_buffer.push(current_message[i]);
    }

    current_message.clear();
  }

  return ret;
}

size_t SoundTransfer::get_chunk_size() {
  return p->max_message_length * strategy->bits_per_frame() / 8;
}

SoundTransfer::~SoundTransfer() {
  delete sm;
  delete input_buffer;
  delete output_buffer;
}
