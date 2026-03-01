#pragma once
#include "kern/sema/TypeChecker.h"
#include <cstdint>

namespace kern {

enum class IRType : uint8_t {
    I8, I16, I32, I64,
    U8, U16, U32, U64,
    F32, F64,
    Bool, Unit, Unknown,
    Struct
};

inline int irTypeBitWidth(IRType t) {
    switch (t) {
        case IRType::I8:  case IRType::U8:  return 8;
        case IRType::I16: case IRType::U16: return 16;
        case IRType::I32: case IRType::U32: case IRType::F32: return 32;
        case IRType::I64: case IRType::U64: case IRType::F64: return 64;
        case IRType::Bool: return 8;  // stored as byte in registers
        case IRType::Struct: return 64; // pointer-sized for addressing
        default: return 64;
    }
}

inline bool irTypeIsFloat(IRType t) {
    return t == IRType::F32 || t == IRType::F64;
}

inline bool irTypeIsSigned(IRType t) {
    switch (t) {
        case IRType::I8: case IRType::I16: case IRType::I32: case IRType::I64:
        case IRType::F32: case IRType::F64:
            return true;
        case IRType::Struct:
            return false;
        default:
            return false;
    }
}

inline const char* irTypeName(IRType t) {
    switch (t) {
        case IRType::I8:      return "i8";
        case IRType::I16:     return "i16";
        case IRType::I32:     return "i32";
        case IRType::I64:     return "i64";
        case IRType::U8:      return "u8";
        case IRType::U16:     return "u16";
        case IRType::U32:     return "u32";
        case IRType::U64:     return "u64";
        case IRType::F32:     return "f32";
        case IRType::F64:     return "f64";
        case IRType::Bool:    return "bool";
        case IRType::Unit:    return "Unit";
        case IRType::Unknown: return "?";
        case IRType::Struct:  return "struct";
    }
    return "?";
}

inline IRType irTypeFromSemaType(Type t) {
    switch (t) {
        case Type::I8:    return IRType::I8;
        case Type::I16:   return IRType::I16;
        case Type::I32:   return IRType::I32;
        case Type::I64:   return IRType::I64;
        case Type::U8:    return IRType::U8;
        case Type::U16:   return IRType::U16;
        case Type::U32:   return IRType::U32;
        case Type::U64:   return IRType::U64;
        case Type::F32:   return IRType::F32;
        case Type::F64:   return IRType::F64;
        case Type::Bool:  return IRType::Bool;
        case Type::Unit:  return IRType::Unit;
        case Type::Error:  return IRType::Unknown;
        case Type::Struct: return IRType::Struct;
        case Type::Enum:   return IRType::I64;   // enum tag is an integer
        case Type::Union:  return IRType::Struct; // union is stack-allocated like struct
        case Type::Ptr:    return IRType::I64;    // pointers are 64-bit addresses
        case Type::PtrVar: return IRType::I64;    // mutable pointers too
    }
    return IRType::Unknown;
}

} // namespace kern
