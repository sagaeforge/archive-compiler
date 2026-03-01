#include "kern/support/TypeSystem.h"
#include <cassert>
#include <cstring>

namespace kern {

TypeTable::TypeTable(Arena& arena) : arena_(arena) {
    registerPrimitives();
}

void TypeTable::registerPrimitives() {
    // Reserve indices 0-12 for primitive types
    static constexpr PrimitiveKind prims[] = {
        PrimitiveKind::I8,   PrimitiveKind::I16,  PrimitiveKind::I32, PrimitiveKind::I64,
        PrimitiveKind::U8,   PrimitiveKind::U16,  PrimitiveKind::U32, PrimitiveKind::U64,
        PrimitiveKind::F32,  PrimitiveKind::F64,
        PrimitiveKind::Bool, PrimitiveKind::Unit, PrimitiveKind::Error,
    };

    for (auto p : prims) {
        auto* ti = arena_.make<TypeInfo>(TypeInfo::makePrimitive(p));
        types_.push_back(ti);
    }

    // Index 13: Never (bottom type !)
    TypeInfo never_info{};
    never_info.kind = TypeKind::Never;
    auto* never_ti = arena_.make<TypeInfo>(never_info);
    types_.push_back(never_ti);
}

TypeId TypeTable::add(TypeInfo info) {
    auto id = static_cast<TypeId>(types_.size());
    auto* ti = arena_.make<TypeInfo>(info);
    types_.push_back(ti);
    return id;
}

const TypeInfo& TypeTable::get(TypeId id) const {
    assert(id < types_.size() && "TypeId out of range");
    return *types_[id];
}

TypeId TypeTable::makePtr(TypeId pointee, bool is_mutable) {
    // Deduplicate: return existing TypeId if same pointer type exists
    TypeKind target_kind = is_mutable ? TypeKind::PtrMut : TypeKind::Ptr;
    for (uint32_t i = 0; i < types_.size(); ++i) {
        if (types_[i]->kind == target_kind && types_[i]->ptr.pointee == pointee) {
            return static_cast<TypeId>(i);
        }
    }
    TypeInfo ti{};
    ti.kind = target_kind;
    ti.ptr.pointee = pointee;
    ti.ptr.is_mutable = is_mutable;
    return add(ti);
}

TypeId TypeTable::makeFn(std::span<const TypeId> params, TypeId ret) {
    auto* param_copy = arena_.makeArray<TypeId>(params.size());
    std::memcpy(param_copy, params.data(), params.size() * sizeof(TypeId));

    TypeInfo ti{};
    ti.kind = TypeKind::Fn;
    ti.fn.params = param_copy;
    ti.fn.param_count = static_cast<uint32_t>(params.size());
    ti.fn.return_type = ret;
    return add(ti);
}

TypeId TypeTable::makeStruct(std::string_view name, std::span<const FieldInfo> fields) {
    auto* field_copy = arena_.makeArray<FieldInfo>(fields.size());
    uint32_t offset = 0;
    uint32_t max_align = 1;

    for (size_t i = 0; i < fields.size(); ++i) {
        field_copy[i] = fields[i];
        uint32_t field_size = sizeOf(fields[i].type);
        uint32_t field_align = alignOf(fields[i].type);
        // Align offset
        offset = (offset + field_align - 1) & ~(field_align - 1);
        field_copy[i].offset = static_cast<int32_t>(offset);
        offset += field_size;
        if (field_align > max_align) max_align = field_align;
    }
    // Pad total size to alignment
    uint32_t total_size = (offset + max_align - 1) & ~(max_align - 1);

    TypeInfo ti{};
    ti.kind = TypeKind::Struct;
    ti.struct_.name = name;
    ti.struct_.fields = field_copy;
    ti.struct_.field_count = static_cast<uint32_t>(fields.size());
    ti.struct_.size = total_size;
    ti.struct_.align = max_align;
    return add(ti);
}

TypeId TypeTable::makeEnum(std::string_view name,
                           std::span<const std::string_view> variant_names,
                           std::span<const int64_t> values) {
    assert(variant_names.size() == values.size());
    auto count = static_cast<uint32_t>(variant_names.size());

    auto* name_copy = arena_.makeArray<std::string_view>(count);
    std::memcpy(name_copy, variant_names.data(), count * sizeof(std::string_view));

    auto* val_copy = arena_.makeArray<int64_t>(count);
    std::memcpy(val_copy, values.data(), count * sizeof(int64_t));

    TypeInfo ti{};
    ti.kind = TypeKind::Enum;
    ti.enum_.name = name;
    ti.enum_.names = name_copy;
    ti.enum_.values = val_copy;
    ti.enum_.variant_count = count;
    return add(ti);
}

TypeId TypeTable::makeUnion(std::string_view name, std::span<const VariantInfo> variants) {
    auto count = static_cast<uint32_t>(variants.size());
    auto* var_copy = arena_.makeArray<VariantInfo>(count);
    std::memcpy(var_copy, variants.data(), count * sizeof(VariantInfo));

    TypeInfo ti{};
    ti.kind = TypeKind::Union;
    ti.union_.name = name;
    ti.union_.variants = var_copy;
    ti.union_.variant_count = count;
    return add(ti);
}

TypeId TypeTable::makeArrayType(TypeId element, uint32_t count) {
    // Deduplicate: search for existing array type with same element + count
    for (size_t i = 0; i < types_.size(); ++i) {
        const auto& t = *types_[i];
        if (t.kind == TypeKind::Array && t.array.element == element && t.array.count == count) {
            return static_cast<TypeId>(i);
        }
    }
    TypeInfo ti{};
    ti.kind = TypeKind::Array;
    ti.array.element = element;
    ti.array.count = count;
    return add(ti);
}

uint32_t TypeTable::sizeOf(TypeId id) const {
    const auto& ti = get(id);
    switch (ti.kind) {
    case TypeKind::Primitive:
        switch (ti.primitive.prim) {
        case PrimitiveKind::I8:  case PrimitiveKind::U8:  case PrimitiveKind::Bool: return 1;
        case PrimitiveKind::I16: case PrimitiveKind::U16: return 2;
        case PrimitiveKind::I32: case PrimitiveKind::U32: case PrimitiveKind::F32:  return 4;
        case PrimitiveKind::I64: case PrimitiveKind::U64: case PrimitiveKind::F64:  return 8;
        case PrimitiveKind::Unit: return 0;
        case PrimitiveKind::Error: return 0;
        }
        return 0;
    case TypeKind::Struct:
        return ti.struct_.size;
    case TypeKind::Enum:
        return 8;  // enum tag is i64
    case TypeKind::Union: {
        // tag(8) + max payload size, aligned to 8
        uint32_t max_payload = 0;
        for (uint32_t i = 0; i < ti.union_.variant_count; ++i) {
            if (ti.union_.variants[i].payload_type != INVALID_TYPE) {
                uint32_t ps = sizeOf(ti.union_.variants[i].payload_type);
                if (ps > max_payload) max_payload = ps;
            }
        }
        return 8 + ((max_payload + 7) & ~7u);
    }
    case TypeKind::Ptr:
    case TypeKind::PtrMut:
        return 8;
    case TypeKind::Fn:
        return 8;  // function pointer
    case TypeKind::Array:
        return sizeOf(ti.array.element) * ti.array.count;
    case TypeKind::TypeVar:
        return 0;  // unknown at this point
    case TypeKind::Never:
        return 0;  // never type has no size
    }
    return 0;
}

uint32_t TypeTable::alignOf(TypeId id) const {
    const auto& ti = get(id);
    switch (ti.kind) {
    case TypeKind::Primitive:
        switch (ti.primitive.prim) {
        case PrimitiveKind::I8:  case PrimitiveKind::U8:  case PrimitiveKind::Bool: return 1;
        case PrimitiveKind::I16: case PrimitiveKind::U16: return 2;
        case PrimitiveKind::I32: case PrimitiveKind::U32: case PrimitiveKind::F32:  return 4;
        case PrimitiveKind::I64: case PrimitiveKind::U64: case PrimitiveKind::F64:  return 8;
        case PrimitiveKind::Unit: case PrimitiveKind::Error: return 1;
        }
        return 1;
    case TypeKind::Struct:
        return ti.struct_.align;
    case TypeKind::Enum:
    case TypeKind::Union:
    case TypeKind::Ptr:
    case TypeKind::PtrMut:
    case TypeKind::Fn:
        return 8;
    case TypeKind::Array:
        return alignOf(ti.array.element);
    case TypeKind::TypeVar:
        return 1;
    case TypeKind::Never:
        return 1;
    }
    return 1;
}

uint32_t TypeTable::bitWidth(TypeId id) const {
    return sizeOf(id) * 8;
}

bool TypeTable::isFloat(TypeId id) const {
    const auto& ti = get(id);
    if (ti.kind != TypeKind::Primitive) return false;
    return ti.primitive.prim == PrimitiveKind::F32 ||
           ti.primitive.prim == PrimitiveKind::F64;
}

bool TypeTable::isSigned(TypeId id) const {
    const auto& ti = get(id);
    if (ti.kind != TypeKind::Primitive) return false;
    switch (ti.primitive.prim) {
    case PrimitiveKind::I8: case PrimitiveKind::I16:
    case PrimitiveKind::I32: case PrimitiveKind::I64:
    case PrimitiveKind::F32: case PrimitiveKind::F64:
        return true;
    default:
        return false;
    }
}

bool TypeTable::isInteger(TypeId id) const {
    const auto& ti = get(id);
    if (ti.kind != TypeKind::Primitive) return false;
    switch (ti.primitive.prim) {
    case PrimitiveKind::I8:  case PrimitiveKind::I16:
    case PrimitiveKind::I32: case PrimitiveKind::I64:
    case PrimitiveKind::U8:  case PrimitiveKind::U16:
    case PrimitiveKind::U32: case PrimitiveKind::U64:
        return true;
    default:
        return false;
    }
}

bool TypeTable::isPrimitive(TypeId id) const {
    return id < PRIMITIVE_COUNT;
}

const char* TypeTable::name(TypeId id) const {
    const auto& ti = get(id);
    switch (ti.kind) {
    case TypeKind::Primitive:
        switch (ti.primitive.prim) {
        case PrimitiveKind::I8:    return "i8";
        case PrimitiveKind::I16:   return "i16";
        case PrimitiveKind::I32:   return "i32";
        case PrimitiveKind::I64:   return "i64";
        case PrimitiveKind::U8:    return "u8";
        case PrimitiveKind::U16:   return "u16";
        case PrimitiveKind::U32:   return "u32";
        case PrimitiveKind::U64:   return "u64";
        case PrimitiveKind::F32:   return "f32";
        case PrimitiveKind::F64:   return "f64";
        case PrimitiveKind::Bool:  return "bool";
        case PrimitiveKind::Unit:  return "Unit";
        case PrimitiveKind::Error: return "Error";
        }
        return "?";
    case TypeKind::Struct: return "struct";
    case TypeKind::Enum:   return "enum";
    case TypeKind::Union:  return "union";
    case TypeKind::Ptr:    return "Ptr";
    case TypeKind::PtrMut: return "PtrVar";
    case TypeKind::Fn:     return "Fn";
    case TypeKind::Array:  return "Array";
    case TypeKind::TypeVar: return "TypeVar";
    case TypeKind::Never:  return "!";
    }
    return "?";
}

} // namespace kern
