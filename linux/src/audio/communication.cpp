#include "communication.h"
#include "../config.h"
#include "../util.h"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>
#include <sys/types.h>
#include <vector>

Communication *comm_instance = NULL;

Communication::Communication(AudioControl *audio, const uint8_t address[2]) {
  comm_instance = this;
  this->address[0] = address[0];
  this->address[1] = address[1];
  this->audio = audio;

  init_transfer();
  received_data = std::vector<uint8_t>();
}

Communication::~Communication() { delete transfer; }

void Communication::init_transfer() {
  SoundTransfer::LOG_LEVEL = LogLevel::warning;
  auto config = AuthConfig::instance().getConfig();
  auto protocolConfig = config["Protocol"].is_table()
                            ? *config["Protocol"].as_table()
                            : toml::table{};
  auto modulationConfig = config["Modulation"].is_table()
                              ? *config["Modulation"].as_table()
                              : toml::table{};

  int fftSize = protocolConfig["fftSize"].value_or(1024);
  float markerF1 =
      protocolConfig["markerF1"].value_or(15000);
  float markerF2 =
      protocolConfig["markerF2"].value_or(17000);

  float lowest_trength;

  p = *createProtocolConfig(fftSize, 48000, markerF1, markerF2);
  p.lowest_strength =
      protocolConfig["lowestStrength"].value_or(-100);
  p.strength_threshold =
      protocolConfig["strengthThreshold"].value_or(-60);
  p.max_message_length = protocolConfig["chunkSize"].value_or(64);

  std::string modulationType =
      modulationConfig["type"].value_or("2tone");

  ModulationStrategy *strategy;

  if (modulationType == "2tone") {
    int start = modulationConfig["startFrequency"].value_or(15000);
    int spacing = modulationConfig["spacing"].value_or(5);
    int bitsPerFrame =
        modulationConfig["bitsPerFrame"].value_or(4);
    strategy = new TwoTonePerBitModulationStrategy(&p, start, bitsPerFrame, spacing);
  } else if (modulationType == "MFSK") {
    int start = modulationConfig["startFrequency"].value_or(15000);
    int spacing = modulationConfig["spacing"].value_or(5);
    int regionSize = modulationConfig["regionSize"].value_or(4);
    int numRegions = modulationConfig["numRegions"].value_or(2);

    strategy =
        new MFSKModulationStrategy(&p, start, spacing, regionSize, numRegions);
  } else if (modulationType == "simple") {
    int f1 = modulationConfig["f1"].value_or(p.f1);
    int f2 = modulationConfig["f2"].value_or(p.f2);
    strategy = new SimpleTwoBitModulationStrategy(f1, f2);
  } else {
    std::cerr << "Unknown modulation type" << std::endl;
    return;
  }

  transfer = new SoundTransfer(strategy, &p);

  audio->setInputBuffer(transfer->get_input_buffer());
  audio->setOutputBuffer(transfer->get_output_buffer());
  audio->start_loop();
  audio->openStreams();
  transfer->run();
}

bool Communication::is_valid(std::vector<uint8_t> &data) {
  assert(data.size() > 3);
  // broadcast
  if (data[0] == 0 && data[1] == 0) {
    return true;
  }
  // to me
  if (data[0] == (uint8_t)address[0] && data[1] == (uint8_t)address[1]) {
    return true;
  }
  return false;
}

int Communication::get_data(std::vector<uint8_t> &out) {
  out.resize(received_data.size());
  for (std::size_t i = 0; i < received_data.size(); i++) {
    out[i] = received_data[i];
  }

  received_data.clear();
  return out.size();
}

static uint16_t crc16(const uint8_t *data, size_t length) {
    uint16_t crc = 0xFFFF;

    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];

        for (int j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }

    return crc;
}

int Communication::send_message(std::vector<uint8_t> &data,
                                const uint8_t to[2]) {
  assert(data.size() > 0);
  std::vector<uint8_t> message(4);
  std::vector<uint8_t> myaddr = AuthConfig::instance().getAddress();
  message[0] = to[0];
  message[1] = to[1];
  message[2] = myaddr[0];
  message[3] = myaddr[1];
  message.insert(message.end(), data.begin(), data.end());

  uint16_t crc = crc16(message.data(), message.size());

  message.push_back((crc >> 8) & 0xFF);
  message.push_back(crc & 0xFF);

  transfer->send(message);

  return 0;
}

void Communication::stop() {
  audio->end_loop();
  transfer->stop();
}

int Communication::send_broadcast(std::vector<uint8_t> &data) {
  assert(data.size() > 0);
  return send_message(data, BROADCAST_ADDRESS);
}

int Communication::recv(int len) {
  std::vector<uint8_t> a;
  received_data.clear();
  while (1) {
    a = transfer->recv(4, true);

    if (a.empty()) {
      return -1;
    }

    if (is_valid(a)) {
      break;
    } else {
      // std::cerr << "Invalid message to " << std::hex << a[0] << a[1]
      //           << " from: " << a[2] << a[3] << std::endl;
    }
  }

  received_data.insert(received_data.end(), a.begin(), a.end());
  a.clear();
  a = transfer->recv(len - 4, false);
  received_data.insert(received_data.end(), a.begin(), a.end());
  return 0;
}
