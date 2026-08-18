#include "kern/debug/ValueInspector.h"
#include <cstring>
#include <sstream>

namespace kern {

ValueInspector::ValueInspector(const TypeTable& types) : types_(types) {}

size_t ValueInspector::sizeOf(TypeId type) const {
    if (type >= types_.size()) return 0;
    return types_.sizeOf(type);
}

std::string ValueInspector::format(TypeId type, const uint8_t* bytes,
                                   size_t size) const {
    if (type >= types_.size()) {
        return "<unknown type>";
    }

    const auto& info = types_.get(type);

    switch (info.kind) {
        case TypeKind::Primitive:
            return formatPrimitive(info, bytes, size);
        case TypeKind::Ptr:
        case TypeKind::PtrMut:
            return formatPointer(info, bytes);
        case TypeKind::Struct:
            return formatStruct(info, bytes, size);
        case TypeKind::Enum:
            if (size >= 8) {
                int64_t tag = 0;
                std::memcpy(&tag, bytes, 8);
                return "enum(" + std::to_string(tag) + ")";
            }
            return "<enum>";
        case TypeKind::Union:
            if (size >= 8) {
                int64_t tag = 0;
                std::memcpy(&tag, bytes, 8);
                return "union(tag=" + std::to_string(tag) + ")";
            }
            return "<union>";
        case TypeKind::Fn:
            if (size >= 8) {
                uint64_t addr = 0;
                std::memcpy(&addr, bytes, 8);
                std::ostringstream oss;
                oss << "fn@0x" << std::hex << addr;
                return oss.str();
            }
            return "<fn>";
        default:
            return "<unsupported type>";
    }
}

std::string ValueInspector::formatPrimitive(const TypeInfo& info,
                                            const uint8_t* bytes,
                                            size_t size) const {
    auto pk = info.primitive.prim;

    switch (pk) {
        case PrimitiveKind::Bool: {
            if (size < 1) return "<bool?>";
            return bytes[0] ? "true" : "false";
        }
        case PrimitiveKind::I8: {
            if (size < 1) return "<i8?>";
            int8_t v = 0;
            std::memcpy(&v, bytes, 1);
            return std::to_string(v);
        }
        case PrimitiveKind::I16: {
            if (size < 2) return "<i16?>";
            int16_t v = 0;
            std::memcpy(&v, bytes, 2);
            return std::to_string(v);
        }
        case PrimitiveKind::I32: {
            if (size < 4) return "<i32?>";
            int32_t v = 0;
            std::memcpy(&v, bytes, 4);
            return std::to_string(v);
        }
        case PrimitiveKind::I64: {
            if (size < 8) return "<i64?>";
            int64_t v = 0;
            std::memcpy(&v, bytes, 8);
            return std::to_string(v);
        }
        case PrimitiveKind::U8: {
            if (size < 1) return "<u8?>";
            return std::to_string(bytes[0]);
        }
        case PrimitiveKind::U16: {
            if (size < 2) return "<u16?>";
            uint16_t v = 0;
            std::memcpy(&v, bytes, 2);
            return std::to_string(v);
        }
        case PrimitiveKind::U32: {
            if (size < 4) return "<u32?>";
            uint32_t v = 0;
            std::memcpy(&v, bytes, 4);
            return std::to_string(v);
        }
        case PrimitiveKind::U64: {
            if (size < 8) return "<u64?>";
            uint64_t v = 0;
            std::memcpy(&v, bytes, 8);
            return std::to_string(v);
        }
        case PrimitiveKind::F32: {
            if (size < 4) return "<f32?>";
            float v = 0;
            std::memcpy(&v, bytes, 4);
            std::ostringstream oss;
            oss << v << "f";
            return oss.str();
        }
        case PrimitiveKind::F64: {
            if (size < 8) return "<f64?>";
            double v = 0;
            std::memcpy(&v, bytes, 8);
            std::ostringstream oss;
            oss << v;
            return oss.str();
        }
        case PrimitiveKind::Unit:
            return "()";
        case PrimitiveKind::Isize: {
            if (size < 8) return "<isize?>";
            int64_t v = 0;
            std::memcpy(&v, bytes, 8);
            return std::to_string(v);
        }
        case PrimitiveKind::Usize: {
            if (size < 8) return "<usize?>";
            uint64_t v = 0;
            std::memcpy(&v, bytes, 8);
            return std::to_string(v);
        }
        default:
            return "<primitive?>";
    }
}

std::string ValueInspector::formatPointer(const TypeInfo& info,
                                          const uint8_t* bytes) const {
    uint64_t addr = 0;
    std::memcpy(&addr, bytes, 8);

    std::ostringstream oss;
    if (info.kind == TypeKind::PtrMut) {
        oss << "Ptr<var ";
    } else {
        oss << "Ptr<";
    }
    auto pointee = info.ptr.pointee;
    if (pointee < types_.size()) {
        oss << types_.name(pointee);
    } else {
        oss << "?";
    }
    oss << ">(0x" << std::hex << addr << ")";
    return oss.str();
}

std::string ValueInspector::formatStruct(const TypeInfo& info,
                                         const uint8_t* /* bytes */,
                                         size_t /* size */) const {
    std::ostringstream oss;
    oss << "struct";
    oss << "{" << info.struct_.field_count << " fields}";
    return oss.str();
}

std::string ValueInspector::formatLocal(const LocalVarInfo& var, uint64_t rbp,
                                        const MemoryReader& read_memory) const {
    size_t sz = sizeOf(var.type);
    if (sz == 0) sz = 8;  // default to 8 bytes for unknown types

    std::vector<uint8_t> buf(sz);
    uint64_t addr = rbp + static_cast<uint64_t>(static_cast<int64_t>(var.stack_offset));

    if (!read_memory(addr, buf.data(), sz)) {
        return "<read failed>";
    }

    return format(var.type, buf.data(), sz);
}

} // namespace kern
