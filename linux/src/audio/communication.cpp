#include "communication.h"
#include <cstdint>
#include <cstdio>
#include <iostream>

#define PROTOCOL GGWAVE_PROTOCOL_ULTRASOUND_FASTEST

Communication* comm_instance = NULL;

Communication::Communication(AudioControl* audio) {
  ggwave_setLogFile(stderr);
  ggwave_Parameters parameters = ggwave_getDefaultParameters();
  parameters.sampleFormatInp = audio->getInputSampleFormat();
  parameters.sampleFormatOut = audio->getOutputSampleFormat();
  parameters.sampleRateInp = audio->getInputSampleRate();
  parameters.sampleRateOut = audio->getOutputSampleRate();
  parameters.payloadLength = -1;
  ggWave = new GGWave(parameters);
  comm_instance = this;

  audio->setRequiredBufferSize(ggWave->samplesPerFrame()*ggWave->sampleSizeInp());
  audio->capture_callback = [](uint8_t* samples, std::size_t size){
    comm_instance->samples_received(samples, size);
  };


  received_data = std::vector<uint8_t>();
}

Communication::~Communication() {
  delete ggWave;
}

void Communication::samples_received(uint8_t* samples, std::size_t sample_size)
{
  bool success = ggWave->decode(samples, sample_size);
  GGWave::TxRxData message;

  if (!success) {
    std::cerr << "Failed to decode message" << std::endl;
  } else{
    int len = ggWave->rxTakeData(message);
    if (len > 0) {
      std::cout << "Decoded message size: " << message.size();
      received_data.insert(received_data.end(), message.begin(), message.end());
      if (receive_callback != NULL) {
        receive_callback();
      }
    }
  }
}

int Communication::encode_message(std::vector<uint8_t> &message) {
  ggWave->init(message.size(), (const char *) message.data(), PROTOCOL, 50);
  std::size_t len = ggWave->encode();

  waveform.resize(len);
  memcpy(waveform.data(), ggWave->txWaveform(), len);

  return len;
}

std::vector<uint8_t> Communication::get_waveform() {
  return waveform;
}

int Communication::get_data(std::vector<uint8_t> &out){
  out.resize(received_data.size());
  for (std::size_t i = 0; i < received_data.size(); i++)
  {
    out[i] = received_data[i];
  }

  received_data.clear();
  return out.size();
}
