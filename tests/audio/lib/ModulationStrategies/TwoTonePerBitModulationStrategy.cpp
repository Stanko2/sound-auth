#include "../modulation.h"
#include "ModulationStrategies/modulationStrategy.h"
#include <cassert>
#include <string>
#include <vector>

TwoTonePerBitModulationStrategy::TwoTonePerBitModulationStrategy(
    const std::vector<int> &frequencies) {
  assert(frequencies.size() % 2 == 0);

  this->frequencies = frequencies;
}

std::string
TwoTonePerBitModulationStrategy::modulate(const std::vector<bool> &data) {
  int done = 0;
  std::string out = "";
  while (done < data.size()) {
    for (int i = 0; i < frequencies.size() / 2; i++) {
      if (data[done]) {
        out += std::to_string(bin_to_freq(frequencies[2 * i + 1]));
      } else {
        out += std::to_string(bin_to_freq(frequencies[2 * i]));
      }
      out += ",";
      done++;
      if (done >= data.size()) {
        break;
      }
    }

    out = out.substr(0, out.size() - 1);
    out += "|";
  }

  out = out.substr(0, out.size() - 1);
  return out;
}

std::vector<bool> TwoTonePerBitModulationStrategy::demodulate(int offset) {
  Spectrum *s = sm->get_spectrum(offset);
  std::vector<bool> out(frequencies.size() / 2);

  // for (int i = 0; i < frequencies.size(); i++) {
  //   std::cout << s->strength(frequencies[i]) << " ";
  // }
  // std::cout << std::endl;

  for (int i = 0; i < out.size(); i++) {
    out[i] =
        s->strength(frequencies[2 * i + 1]) >= s->strength(frequencies[2 * i]);
    // std::cout << out[i];
  }
  // std::cout << ' ';

  delete s;
  return out;
}

void TwoTonePerBitModulationStrategy::print(std::ostream &os) const {
  os << "TwoTonePerBitModulationStrategy [Frequencies: ";
  for (size_t i = 0; i < frequencies.size(); ++i) {
    os << bin_to_freq(frequencies[i]) << (i == frequencies.size() - 1 ? "" : ", ");
  }
  os << "]";
}
