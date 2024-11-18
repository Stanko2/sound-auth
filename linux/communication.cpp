#include "communication.h"
#include <iostream>

Communication::Communication() {
  ggwave = new GGWave(GGWave::Parameters {
    -1,
    GGWave::kDefaultSampleRate,
    GGWave::kDefaultSampleRate,
    GGWave::kDefaultSampleRate,
    GGWave::kDefaultSamplesPerFrame,
    GGWave::kDefaultSoundMarkerThreshold,
    GGWAVE_SAMPLE_FORMAT_I16,
    GGWAVE_SAMPLE_FORMAT_I16,
    GGWAVE_OPERATING_MODE_RX_AND_TX
  });

  size_t sample_buffer_size = ggwave->samplesPerFrame() * ggwave->sampleSizeInp() * 32;

  received_samples = std::vector<uint16_t>(sample_buffer_size);
  received_data = std::vector<uint8_t>(0);
}

Communication::~Communication() {
}

void Communication::samples_received(uint16_t* samples, size_t sample_size)
{
  uint16_t max = 0;
  for (size_t i = 0; i < sample_size; i++)
  {
    received_samples.push_back(samples[i]);
    max = std::max(max, samples[i]);
  }

  size_t required_size = ggwave->samplesPerFrame() * ggwave->sampleSizeInp();

  if (received_samples.size() > required_size) {
    int ret = ggwave->decode(received_samples.data(), received_samples.size() * sizeof(uint16_t));

    if (!ret) {
      std::cerr << "Failed to decode message" << std::endl;
    } else {
      std::cout << "Decoded message: " << ggwave->rxData().size();

      received_data.insert(received_data.end(), ggwave->rxData().data(), ggwave->rxData().data() + ggwave->rxData().size());
      // for (size_t i = 0; i < received_data.size(); i++)
      // {
      //   fprintf(stdout, "%d ", received_data[i]);
      // }
      received_data.clear();
    }
    std::cout << "Max " << max << ' ' << ret;
    received_samples.clear();
    std::cout << std::endl;
  }
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
