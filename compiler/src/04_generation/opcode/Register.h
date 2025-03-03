#pragma once

#include <array>
#include <cstdint>
#include <cstring>

namespace nugdev::compiler::generation {

#define REGISTER_SIZE 8

class RegisterData {

  public:
    RegisterData() {}
    template <typename T> RegisterData(T value) { set(value); }

  public:
    template <typename T> T get() {
        static_assert(sizeof(T) == 8, "RegisterData can only hold 8 bytes");
        return *reinterpret_cast<T *>(m_value.data());
    }
    template <typename T> void set(T value) {
        static_assert(sizeof(T) == 8, "RegisterData can only hold 8 bytes");
        memory_copy(*this, value);
    }
    bool compare(const RegisterData &data) { return memory_compare(*this, data); }

  public:
    static void memory_set(RegisterData &data, std::uint8_t value) { std::memset(data.m_value.data(), value, 8); }
    template <typename T> static void memory_copy(RegisterData &data, T value) {
        static_assert(sizeof(T) == 8, "RegisterData can only hold 8 bytes");
        std::memcpy(data.m_value.data(), &value, 8);
    }
    template <typename T> static bool memory_compare(RegisterData &data, T value) {
        if constexpr (std::is_same_v<T, RegisterData>) {
            return std::memcmp(data.m_value.data(), value.m_value.data(), REGISTER_SIZE);
        }

        if constexpr (sizeof(T) != 8) {
            throw std::invalid_argument("RegisterData can only hold 8 bytes");
        }

        return std::memcmp(data.m_value.data(), &value, 8);
    }

  private:
    std::array<std::uint8_t, REGISTER_SIZE> m_value;
};

class RegisterTag {};

class Register {
  public:
    Register(RegisterTag tag, RegisterData data);

  private:
    RegisterTag m_index;
    RegisterData m_data;
};

} // namespace nugdev::compiler::generation
