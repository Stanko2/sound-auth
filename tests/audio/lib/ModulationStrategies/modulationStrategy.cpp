#include "modulationStrategy.h"
#include "../modulation.h"

void ModulationStrategy::Init(SignalModulation* s, ProtocolConfig* p) {
  this->sm = s;
  this->p = p;
}

float ModulationStrategy::bin_to_freq(const int bin) const  {
  float delta = (float)p->sample_rate / (float)p->N;
  return bin * delta;
}

int ModulationStrategy::freq_to_bin(const float freq) const {
  return freq * p->N / p->sample_rate;
}

void ModulationStrategy::print(std::ostream& os) const {
  os << "ModulationStrategy";
}

int ModulationStrategy::bits_per_frame() {
  return 0;
}
