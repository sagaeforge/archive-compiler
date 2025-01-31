#pragma once

namespace nugdev::compiler::lib {

class Singleton {
public:
  virtual ~Singleton() = default;

  static Singleton &instance() {
    static Singleton instance;
    return instance;
  }
};

} // namespace nugdev::compiler::lib