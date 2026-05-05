#include "../modulation.h"
#include "./modulationStrategy.h"
#include "../waveforms.h"
#include <iostream>
#include <string>
#include <vector>

SimpleTwoBitModulationStrategy::SimpleTwoBitModulationStrategy(int f1, int f2) {
  this->f1 = bin_to_freq(f1);
  this->f2 = bin_to_freq(f2);
}

std::string
SimpleTwoBitModulationStrategy::modulate(const std::vector<bool> &data) {
  std::string frequencies[2] = {std::to_string(f1), std::to_string(f2)};
  std::string waveformString = "";
  std::cout << "Modulate " << data.size() << std::endl;
  for (int i = 0; i < data.size(); i += 2) {
    bool added = false;
    for (int j = 0; j < 2; j++) {
      if (data[i + j]) {
        waveformString += frequencies[j] + ",";
        added = true;
      }
    }
    if (!added) {
      waveformString += "0|";
    } else {
      waveformString += "|";
    }
  }
  return waveformString;
}

// This is not working correctly and needs tweaking of many constants - not
// effective
bool SimpleTwoBitModulationStrategy::is_present(int frame_offset, int f) {
  int start = frame_offset - 80;
  int end = frame_offset + 80;
  float local_max = -100, local_min = 0;
  bool decaying = true;

  float last_strength = 0;
  float strength_start, strength_end;
  for (int i = start; i <= end; i += 8) {
    Spectrum *s = sm->get_spectrum(i);
    float strength = s->strength(f);
    if (i == start)
      strength_start = strength;
    if (i == end)
      strength_end = strength;

    if (i < frame_offset && last_strength < strength)
      decaying = false;

    if (strength > local_max)
      local_max = strength;

    if (strength < local_min)
      local_min = strength;

    last_strength = strength;
    delete s;
  }

  // ak sme pod thresholdom, frekvencia tam nie je
  if (local_max <= -40) {
    return false;
  }

  if (decaying && local_max - local_min > 2) {
    return false;
  }

  if (strength_start - strength_end > 3)
    return false;

  if (strength_level != 0 && strength_level - local_max > 10)
    return false;

  strength_level = local_max;
  return true;
}

std::vector<bool> SimpleTwoBitModulationStrategy::demodulate(int offset) {
  std::vector<bool> out(2);
  out[0] = is_present(offset, f1);
  out[1] = is_present(offset, f2);

  return out;
}

void SimpleTwoBitModulationStrategy::print(std::ostream &os) const {
  os << "SimpleTwoBitModulationStrategy [f1: " << bin_to_freq(f1) << "Hz, f2: " << bin_to_freq(f2)
     << "Hz]";
}

bool SimpleTwoBitModulationStrategy::has_noise(Spectrum* s) {
  return s->strength(f1) < p->strength_threshold && s->strength(f2) < p->strength_threshold;
}

int SimpleTwoBitModulationStrategy::bits_per_frame() {
  return 2;
}
