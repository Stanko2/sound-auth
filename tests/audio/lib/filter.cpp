#include "filter.h"
#include "waveforms.h"
#include <cmath>

CombFilter::CombFilter(float sample_rate, float target, float decay_seconds) {
  delaySamples = (int)(sample_rate / target);
  buffer.resize(delaySamples, 0.0f);
  writeIndex = 0;

  float d = std::max(0.001f, decay_seconds);
  gain = std::pow(0.001f, (float)delaySamples / (d * sample_rate));
}

float CombFilter::process(float input) {
  float delayed = buffer[writeIndex];
  float output = input + (gain * delayed);

  buffer[writeIndex] = output;

  writeIndex = (writeIndex + 1) % delaySamples;

  return output;
}

GaussianWindow::GaussianWindow(int N, float alpha) {
  double center = (N - 1) / 2.0;
  generated_window.resize(N);

  for (int n = 0; n < N; ++n) {
    double val =
        (n - center) / (alpha * (N - 1) / 2.0); // Simplified internal term
    // Alternatively, using the standard formula:
    double numerator = n - center;
    double denominator = (N - 1) / 2.0;
    generated_window[n] =
        std::exp(-0.5 * std::pow(alpha * (numerator / denominator), 2));
  }
}

float GaussianWindow::apply(float input, int i) {
  return input * generated_window[i];
}

HannWindow::HannWindow(int N) {
  this->N = N;
}

float HannWindow::apply(float input, int i) {
  return 0.5 * (1 - cos(2 * PI * i / (N - 1)));
}
