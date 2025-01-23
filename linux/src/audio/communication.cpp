#include "communication.h"
#include <ggwave/ggwave.h>
#include <iostream>

Communication::Communication(AudioControl* audio) {
  ggwave_setLogFile(stderr);
  ggwave_Parameters parameters = ggwave_getDefaultParameters();
  parameters.sampleFormatInp = audio->getInputSampleFormat();
  parameters.sampleFormatOut = audio->getOutputSampleFormat();
  parameters.sampleRateInp = audio->getInputSampleRate();
  parameters.sampleRateOut = audio->getOutputSampleRate();
  parameters.payloadLength = -1;
  ggWave = new GGWave(parameters);
  instance = this;

  audio->setRequiredBufferSize(ggWave->samplesPerFrame()*ggWave->sampleSizeInp());
  audio->capture_callback = [](uint8_t* samples, size_t size){
    instance->samples_received(samples, size);
  };


  received_data = std::vector<uint8_t>();
}

Communication::~Communication() {
  delete ggWave;
}

void Communication::samples_received(uint8_t* samples, size_t sample_size)
{
  bool success = ggWave->decode(samples, sample_size);
  // std::cout << "decoding " << std::endl;
  GGWave::TxRxData message;

  if (!success) {
    std::cerr << "Failed to decode message" << std::endl;
  } else{
    int len = ggWave->rxTakeData(message);
    if (len > 0) {
      std::cout << "Decoded message size: " << message.size();
      received_data.insert(received_data.end(), message.begin(), message.end());
    }
  }
  // std::cout << std::endl;
}

int Communication::encode_message(std::vector<uint8_t> &message) {
  ggWave->init(message.size(), (const char *) message.data(), GGWAVE_PROTOCOL_ULTRASOUND_FASTEST, 25);
  size_t len = ggWave->encode();

  waveform.resize(len);
  memcpy(waveform.data(), ggWave->txWaveform(), len);

  return len;
}

std::vector<uint8_t> Communication::get_waveform() {
  return waveform;
}

int Communication::get_data(std::vector<uint8_t> &out)
{
  out.resize(received_data.size());
  for (size_t i = 0; i < received_data.size(); i++)
  {
    out[i] = received_data[i];
  }

  received_data.clear();
  return out.size();
}
