#include "../modulation.h"

void ModulationStrategy::Init(SignalModulation* s, ProtocolConfig* p) {
  this->sm = s;
  this->p = p;
}

float ModulationStrategy::bin_to_freq(int bin) {
  float delta = (float)p->sample_rate / (float)p->N;
  return bin * delta;
}
