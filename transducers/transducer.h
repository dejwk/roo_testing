#pragma once

#include <string>

class Transducer {
 public:
  Transducer() {}
  virtual ~Transducer() {}

  virtual const std::string& name() const {
    static std::string name = "<unnamed>";
    return name;
  }
};
