#pragma once

#include "RegisterData.hpp"
#include "RegisterTag.h"

namespace nugdev::compiler::generation {

class Register {
  public:
    Register(RegisterTag tag, RegisterData data);

  private:
    RegisterTag m_index;
    RegisterData m_data;
};

} // namespace nugdev::compiler::generation
