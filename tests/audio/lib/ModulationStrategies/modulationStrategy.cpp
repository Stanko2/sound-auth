#include "modulationStrategy.h"
#include "../modulation.h"

void ModulationStrategy::Init(SignalModulation* s, ProtocolConfig* p) {
  this->sm = s;
  this->p = p;
}

float ModulationStrategy::bin_to_freq(int bin) const  {
  float delta = (float)p->sample_rate / (float)p->N;
  return bin * delta;
}

void ModulationStrategy::print(std::ostream& os) const {
  os << "ModulationStrategy";
}

int ModulationStrategy::bits_per_frame() {
  return 0;
}
