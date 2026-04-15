#ifndef MODULATIONSTRATEGY_H
#define MODULATIONSTRATEGY_H

#include <cstdint>
#include <ostream>
#include <string>
#include <vector>

class SignalModulation;
struct ProtocolConfig;

class ModulationStrategy {
protected:
  SignalModulation* sm;
  ProtocolConfig* p;
  float bin_to_freq(int bin) const;
  virtual void print(std::ostream& o) const;
public:
  virtual ~ModulationStrategy() = default;
  void Init(SignalModulation* s, ProtocolConfig* p);
  /*
   * Gets a "frequency string" from the data. Needs to be implemented for
   * different modulation schemes
   */
  virtual std::string modulate(const std::vector<bool> &data) = 0;
  /*
   * Gets data back from measured samples. Can use `s->get_spectrum(offset)`
   * to read measured frequencies
   */
  virtual std::vector<bool> demodulate(int frame_offset) = 0;

  friend std::ostream& operator<<(std::ostream &o, const ModulationStrategy& strategy) {
    strategy.print(o);
    return o;
  }
};


/*
 * Uses 2 frequencies - f1 and f2
 * Data is encoded 2 bits per frame - 1 bit comes from f1 and other from f2
 *
 * Not that accurate and effective - cannot detect if frequency is not present reliably
 * Speed is also really limited
 */
class SimpleTwoBitModulationStrategy : public ModulationStrategy {
private:
  int f1, f2;
  float strength_level;
  bool is_present(int offset, int f);
protected:
  void print(std::ostream& os) const override;

public:
  SimpleTwoBitModulationStrategy(int f1, int f2);
  std::string modulate(const std::vector<bool> &data) override;
  std::vector<bool> demodulate(int frame_offset) override;
};

/*
 * Uses 2 frequencies per bit
 *  - if f1 is present, then bit is 1
 *  - if f2 is present, then bit is 0
 */
class TwoTonePerBitModulationStrategy : public ModulationStrategy {
private:
  std::vector<int> frequencies;
protected:
  void print(std::ostream& os) const override;
public:
  TwoTonePerBitModulationStrategy(const std::vector<int> &frequencies);
  std::string modulate(const std::vector<bool> &data) override;
  std::vector<bool> demodulate(int frame_offset) override;
};


/*
 * Uses 1 of multiple frequencies - determines couple of bits at once
 * by the frequency that is most predominant in spectrum region
 */
class MultiToneModulationStrategy : public ModulationStrategy {
  std::string modulate(const std::vector<bool> &data) override;
  std::vector<bool> demodulate(int frame_offset) override;
protected:
  void print(std::ostream& os) const override;

};

#endif // MODULATIONSTRATEGY_H
