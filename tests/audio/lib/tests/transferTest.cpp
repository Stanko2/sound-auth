#include "transferTest.h"
#include "../modulation.h"
#include "../entry.h"
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <random>
#include <sstream>

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

  out.reserve(length);
  for (int i = 0; i < length; ++i) {
    uint8_t val = static_cast<uint8_t>(rng() & 0xFF);
    out.push_back(val);
  }

  return out;
}

void test_tx(SoundTransfer *t, int messages, int base_seed) {

  size_t msg_len = t->get_chunk_size();
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

    t->send(msg);

    // std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }
}

void test_rx(SoundTransfer *t, int messages, int base_seed) {
  size_t msg_len = t->get_chunk_size();
  std::cout << "Message length: " << msg_len << std::endl;
  auto state = std::make_shared<State>();

  // Helper to format vector as hex string
  auto to_hex = [](const std::vector<uint8_t> &data,
                   bool include_space = true) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (auto b : data) {
      ss << std::setw(2) << static_cast<int>(b);
      if (include_space) {
        ss << " ";
      }
    }
    return ss.str();
  };

  std::ofstream report_file("report.txt");

  // report_file << *t;
  report_file << "------------------------------------------\n";
  report_file << "expected,received,num_errors" << std::endl;

  std::cout << "Test receive started" << std::endl;

  float errors = 0;
  for (int counter = 0; counter < messages; counter++) {
    std::vector<uint8_t> received = t->recv(msg_len, true);

    std::vector<uint8_t> expected = gen_random_msg(counter, msg_len);
    int bits_diff = compare(expected, received);
    errors += bits_diff;

    std::cout << "------------------------------------------" << std::endl;
    std::cout << "Seed: " << counter
              << std::endl;
    std::cout << "Errors: " << bits_diff << " bits" << std::endl;

    std::cout << "EXPECTED: " << to_hex(expected) << std::endl;
    std::cout << "RECEIVED: " << to_hex(received) << std::endl;

    if (report_file.is_open()) {
      report_file << to_hex(expected, false) << "," << to_hex(received, false)
                  << "," << bits_diff << std::endl;
    }
  }

  float total = messages * msg_len * 8;

  std::cout << "Total accuracy: " << (total - errors) / total * 100 << "%" << std::endl;
}
