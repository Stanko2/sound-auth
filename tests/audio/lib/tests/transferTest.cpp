#include "transferTest.h"
#include "../modulation.h"
#include <chrono>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <thread>

// Compare two byte-messages bitwise and return number of differing bits.
// If lengths differ, extra bytes in the longer message count as 8 differing
// bits each.
int compare(const std::vector<uint8_t> &msg1,
            const std::vector<uint8_t> &msg2) {
  size_t len1 = msg1.size();
  size_t len2 = msg2.size();
  size_t minlen = std::min(len1, len2);
  int diff = 0;

  // compare overlapping bytes bitwise using builtin popcount for speed/clarity
  for (size_t i = 0; i < minlen; ++i) {
    uint8_t x = msg1[i] ^ msg2[i];
    diff += __builtin_popcount((unsigned)x);
  }

  // count any extra bytes in the longer message as 8 differing bits each
  if (len1 > minlen)
    diff += 8 * static_cast<int>(len1 - minlen);
  if (len2 > minlen)
    diff += 8 * static_cast<int>(len2 - minlen);

  return diff;
}

// Generates a random message of given length (bytes) seeded by seed.
std::vector<uint8_t> gen_random_msg(int seed, int length) {
  std::vector<uint8_t> out;
  if (length <= 0)
    return out;
  std::mt19937 rng(static_cast<unsigned int>(seed));
  std::uniform_int_distribution<int> dist(0, 255);
  out.reserve(length);
  for (int i = 0; i < length; ++i) {
    out.push_back(static_cast<uint8_t>(dist(rng)));
  }
  return out;
}

void test_tx(SignalModulation *sm, const TxCallback &tx_callback,
             int messages, int msg_len, int base_seed) {

  // Install tx callback into SignalModulation; the modulation layer expects a
  // callback that accepts the waveform (by value). Wrap the provided callback
  // which takes const ref.
  sm->set_tx_callback([tx_callback](waveform w) {
    if (tx_callback)
      tx_callback(w);
  });

  for (int i = 0; i < messages; ++i) {
    int seed = base_seed + i;
    std::vector<uint8_t> msg = gen_random_msg(seed, msg_len);

    // print message contents in hex
    std::ostringstream oss;
    oss << "TX msg[" << i << "] seed=" << seed << " bytes=" << msg.size()
        << " : ";
    for (auto b : msg) {
      oss << std::hex << std::setw(2) << std::setfill('0') << (int)b << "";
    }
    std::cout << oss.str() << std::dec << std::endl;

    sm->transmit_data(msg);

    // std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }

}

void test_rx(ModulationStrategy *strategy, const RxCallback &rx_callback,
             int messages, int msg_len, int base_seed) {
  if (!strategy) {
    std::cerr << "test_rx: strategy is null\n";
    return;
  }

  ProtocolConfig *pc = createProtocolConfig(1024, 48000, 15000, 17000, 3.0f);
  SignalModulation sm(*pc);
  sm.set_strategy(strategy);

  // If no base_seed provided, fall back to 1 for deterministic tests
  int seed0 = base_seed;
  if (seed0 == 0)
    seed0 = 1;

  int counter = 0;

  sm.set_rx_callback([rx_callback, seed0, msg_len, messages,
                      &counter](std::vector<uint8_t> received) {
    int idx = counter;
    if (idx >= messages) {
      // already received expected number of messages; ignore extras
      return;
    }

    int seed = seed0 + idx;
    std::vector<uint8_t> expected = gen_random_msg(seed, msg_len);

    int bits_diff = compare(expected, received);
    std::cout << "RX idx=" << idx << " seed=" << seed
              << " bytes=" << received.size() << " bit_errors=" << bits_diff
              << std::endl;

    if (rx_callback)
      rx_callback(received);

    counter++;
  });

  int wait_ms = std::max(1000, messages * 250);
  std::this_thread::sleep_for(std::chrono::milliseconds(wait_ms));

  delete pc;
}
