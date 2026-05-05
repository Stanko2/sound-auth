#include "communication.h"
#include "../config.h"
#include "../util.h"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <sys/types.h>
#include <vector>

#define PROTOCOL GGWAVE_PROTOCOL_ULTRASOUND_FASTEST

Communication *comm_instance = NULL;

Communication::Communication(AudioControl *audio, const uint8_t address[2]) {
  // ggwave_setLogFile(NULL);
  // ggwave_Parameters parameters = ggwave_getDefaultParameters();
  // parameters.sampleFormatInp = audio->getInputSampleFormat();
  // parameters.sampleFormatOut = audio->getOutputSampleFormat();
  // parameters.sampleRateInp = audio->getInputSampleRate();
  // parameters.sampleRateOut = audio->getOutputSampleRate();
  // parameters.payloadLength = -1;
  // ggWave = new GGWave(parameters);
  // protocol_id = AuthConfig::instance().getProtocol();

  comm_instance = this;
  this->address[0] = address[0];
  this->address[1] = address[1];
  this->audio = audio;

  // audio->setRequiredBufferSize(ggWave->samplesPerFrame()*ggWave->sampleSizeInp());
  // audio->capture_callback = [](uint8_t* samples, std::size_t size){
  //     comm_instance->samples_received(samples, size);
  // };

  init_transfer();
  received_data = std::vector<uint8_t>();
}

Communication::~Communication() {
  // delete ggWave;
  delete transfer;
}

void Communication::init_transfer() {
  p = *createProtocolConfig(1024, 48000, 15000, 17000);
  p.lowest_strength = -100;
  p.strength_threshold = -60;

  ModulationStrategy *strategy =
      new TwoTonePerBitModulationStrategy(p.f1, 4, 5);

  transfer = new SoundTransfer(strategy, &p);

  audio->setInputBuffer(transfer->get_input_buffer());
  audio->setOutputBuffer(transfer->get_output_buffer());
  audio->start_loop();
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

// void Communication::samples_received(uint8_t* samples, std::size_t
// sample_size)
// {
// bool success = ggWave->decode(samples, sample_size);
// GGWave::TxRxData message;
// if (!success) {
//     std::cerr << "Failed to decode message" << std::endl;
// } else{
//     int len = ggWave->rxTakeData(message);
//     if (len > 0) {
//         // std::cout << "Received message: ";
//         // for (auto byte : message) {
//         //     std::cout << std::hex << static_cast<int>(byte) << " ";
//         // }
//         // std::cout << std::dec;
//         if(!is_valid(message)) return;
//         std::cout << std::endl;
//         received_data.insert(received_data.end(), message.begin(),
//         message.end()); if (receive_callback != NULL) {
//             // std::cout << "Message Accepted" << std::endl;
//             receive_callback();
//         }
//     }
// }
// }

// int Communication::encode_message(std::vector<uint8_t> &message) {

//     // ggWave->init(message.size(), (const char *) message.data(),
//     protocol_id, 50);
//     // std::size_t len = ggWave->encode();
//     // assert(len > 0);

//     // waveform.resize(len);
//     // memcpy(waveform.data(), ggWave->txWaveform(), len);

//     // return len;
//     transfer->send(message);
//     return 0;
// }

// std::vector<uint8_t> Communication::get_waveform() {
//     return waveform;
// }

int Communication::get_data(std::vector<uint8_t> &out) {
  out.resize(received_data.size());
  for (std::size_t i = 0; i < received_data.size(); i++) {
    out[i] = received_data[i];
  }

  received_data.clear();
  return out.size();
}

int Communication::send_message(std::vector<uint8_t> &data, const uint8_t to[2]) {
  assert(data.size() > 0);
  std::vector<uint8_t> message(4);
  std::vector<uint8_t> myaddr = AuthConfig::instance().getAddress();
  message[0] = to[0];
  message[1] = to[1];
  message[2] = myaddr[0];
  message[3] = myaddr[1];
  message.insert(message.end(), data.begin(), data.end());

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

void Communication::recv(int len) {
  std::vector<uint8_t> a;
  received_data.clear();
  while (1) {
    a = transfer->recv(4, true);

    if (is_valid(a)) {
      std::cerr << "Valid, returning" << std::endl;
      break;
    } else {
      std::cerr << "Invalid message to " << std::hex << a[0] << a[1] << " from: " << a[2] << a[3] << std::endl;
    }
  }

  received_data.insert(received_data.end(), a.begin(), a.end());
  std::cout << "received: " << vectorToHexString(received_data) << std::endl;
  std::cout << "receiving additional " << len - 4 << " bytes" << std::endl;
  a.clear();
  a = transfer->recv(len - 4, false);
  std::cout << "a: " << vectorToHexString(a) << std::endl;
  received_data.insert(received_data.end(), a.begin(), a.end());
  std::cout << "received: " << vectorToHexString(received_data) << std::endl;
}
