#pragma once

#include <string>

class Transducer {
 public:
  Transducer() {}
  virtual ~Transducer() {}

  virtual const std::string& name() const { return "<unnamed>"; }
};
