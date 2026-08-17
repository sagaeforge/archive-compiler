#pragma once

#include "00_app/tag/Tag.h"

namespace nugdev::compiler::generation {

struct RegisterTag : public Tag {};

template <size_t N = 8> class Register {
  public:
    Register(const RegisterTag &tag) : m_tag(tag) { memset(m_data.data(), 0, N); }
    template <typename T> Register(const RegisterTag &tag, const T &value) : m_tag(tag) {
        // T는 8바이트 이하여야 한다.
        static_assert(sizeof(T) <= N, "T must be 8 bytes or less");
        std::memcpy(m_data.data(), &value, sizeof(T));
    }

    RegisterTag get_tag() const;
    template <typename T> T get_value() const {
        static_assert(sizeof(T) <= N, "T must be 8 bytes or less");
        return *reinterpret_cast<const T *>(m_data.data());
    }
    template <typename T> void set_value(const T &value) {
        static_assert(sizeof(T) <= N, "T must be 8 bytes or less");
        std::memcpy(m_data.data(), &value, sizeof(T));
    }

  private:
    RegisterTag m_tag;
    std::array<char, N> m_data;
};
using UniversalRegister = Register<8>;

} // namespace nugdev::compiler::generation
