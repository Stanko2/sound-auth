#pragma once
#include <vector>
#include <cmath>


// Interface for audio filters
class Filter {
public:
  virtual ~Filter() = default;
  virtual float process(float input) = 0;
};

class WindowFunction {
public:
  virtual float apply(float sample, int i) = 0;
};

// delay L, output y, input x
// y[n] = x[n] + x[n-L]
class CombFilter : public Filter {
private:
  std::vector<float> buffer;
  int writeIndex = 0;
  int delaySamples;
  float gain;

public:
  CombFilter(float sample_rate, float target, float decay);
  float process(float input) override;
};

class GaussianWindow : public WindowFunction {
private:
  std::vector<float> generated_window;

public:
  GaussianWindow(int N, float alpha);
  float apply(float input, int i) override;
};

class HannWindow : public WindowFunction {
private:
  int N;

public:
  HannWindow(int N);
  float apply(float input, int i) override;
};
