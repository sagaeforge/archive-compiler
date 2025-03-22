#pragma once

#include <cstdint>

#include "00_app/lib/UnicodeString.hpp"

namespace nugdev::compiler::ast {

class TypeInfo {
  public:
    lib::String m_literal;
    std::uint32_t m_size;
    bool m_is_primitive;

    bool operator==(const TypeInfo &other) const { return m_literal == other.m_literal && m_size == other.m_size && m_is_primitive == other.m_is_primitive; }
    bool operator!=(const TypeInfo &other) const { return !(*this == other); }
};
extern TypeInfo i8;
extern TypeInfo i16;
extern TypeInfo i32;
extern TypeInfo i64;
extern TypeInfo u8;
extern TypeInfo u16;
extern TypeInfo u32;
extern TypeInfo u64;
extern TypeInfo f8;
extern TypeInfo f16;
extern TypeInfo f32;
extern TypeInfo f64;
extern TypeInfo c8;
extern TypeInfo c16;
extern TypeInfo c32;
extern TypeInfo boolean;
extern TypeInfo none;
extern std::unordered_map<lib::String, TypeInfo &> typeMap;

} // namespace nugdev::compiler::ast