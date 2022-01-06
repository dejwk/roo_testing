#pragma once

#include <cmath>

#include <stdint.h>
#include <functional>

namespace roo_testing_transducers {

class Temperature {
 public:
  static Temperature FromK(double tempK) { return Temperature(tempK); }
  static Temperature FromC(double tempC) { return Temperature(tempC + 273.15); }
  static Temperature FromF(double tempF) { return FromC((tempF - 32.0) / 1.8); }

  double AsK() const { return tempK_; }
  double AsC() const { return tempK_ - 273.15; }
  double AsF() const { return AsC() * 1.8 + 32.0; }

 private:
  Temperature(double tempK) : tempK_(tempK) {}
  double tempK_;
};

inline bool operator==(const Temperature& a, const Temperature& b) {
  return a.AsK() == b.AsK() || (std::isnan(a.AsK()) && std::isnan(b.AsK()));
}

class Thermometer {
 public:
  virtual Temperature read() const = 0;

  virtual ~Thermometer() {}
};

class FixedThermometer : public Thermometer {
 public:
  FixedThermometer(Temperature temp) : temp_(temp) {}
  Temperature read() const override { return temp_; }
  void set(Temperature temp) { temp_ = temp; }

 private:
  Temperature temp_;
};

class VariableThermometer : public Thermometer {
 public:
  VariableThermometer(float* temp) : temp_(temp) {}
  Temperature read() const override { return Temperature::FromC(*temp_); }

 private:
  float* temp_;
};

class FunctionThermometer : public Thermometer {
 public:
  FunctionThermometer(std::function<Temperature()> temp) : temp_(temp) {}
  Temperature read() const override { return temp_(); }

 private:
  std::function<Temperature()> temp_;
};

}  // namespace roo_testing_transducers
