#include "TypeMeta.h"

namespace nugdev::compiler::ast {

TypeInfo i8{"i8", 1, true};
TypeInfo i16{"i16", 2, true};
TypeInfo i32{"i32", 4, true};
TypeInfo i64{"i64", 8, true};
TypeInfo u8{"u8", 1, true};
TypeInfo u16{"u16", 2, true};
TypeInfo u32{"u32", 4, true};
TypeInfo u64{"u64", 8, true};
TypeInfo f8{"f8", 1, true};
TypeInfo f16{"f16", 2, true};
TypeInfo f32{"f32", 4, true};
TypeInfo f64{"f64", 8, true};
TypeInfo c8{"c8", 1, true};
TypeInfo c16{"c16", 2, true};
TypeInfo c32{"c32", 4, true};
TypeInfo boolean{"bool", 1, true};
TypeInfo none_type{"none", 0, true};

std::unordered_map<lib::String, TypeInfo &> typeMap{
    {"i8", i8},   {"i16", i16}, {"i32", i32}, {"i64", i64}, {"u8", u8},   {"u16", u16}, {"u32", u32},      {"u64", u64},        {"f8", f8},
    {"f16", f16}, {"f32", f32}, {"f64", f64}, {"c8", c8},   {"c16", c16}, {"c32", c32}, {"bool", boolean}, {"none", none_type},
};

} // namespace nugdev::compiler::ast
