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

    // Index 14-15: isize/usize (pointer-sized integers, 64-bit on x86-64)
    auto* isize_ti = arena_.make<TypeInfo>(TypeInfo::makePrimitive(PrimitiveKind::Isize));
    types_.push_back(isize_ti);
    auto* usize_ti = arena_.make<TypeInfo>(TypeInfo::makePrimitive(PrimitiveKind::Usize));
    types_.push_back(usize_ti);
}

TypeId TypeTable::add(TypeInfo info) {
    auto id = static_cast<TypeId>(types_.size());
    auto* ti = arena_.make<TypeInfo>(info);
    types_.push_back(ti);
    return id;
}

const TypeInfo& TypeTable::get(TypeId id) const {
    // INVALID_TYPE (UINT32_MAX) can flow from error recovery paths;
    // return the pre-registered Error type instead of crashing.
    if (id >= types_.size()) {
        return *types_[Error];
    }
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

TypeId TypeTable::makeFn(std::span<const TypeId> params, TypeId ret, EffectSet effects) {
    // Deduplicate: return existing TypeId if same function type exists
    for (uint32_t i = 0; i < types_.size(); ++i) {
        if (types_[i]->kind == TypeKind::Fn &&
            types_[i]->fn.param_count == static_cast<uint32_t>(params.size()) &&
            types_[i]->fn.return_type == ret &&
            types_[i]->fn.effects == effects) {
            bool match = true;
            for (uint32_t j = 0; j < params.size(); ++j) {
                if (types_[i]->fn.params[j] != params[j]) { match = false; break; }
            }
            if (match) return static_cast<TypeId>(i);
        }
    }

    auto* param_copy = arena_.makeArray<TypeId>(params.size());
    std::memcpy(param_copy, params.data(), params.size() * sizeof(TypeId));

    TypeInfo ti{};
    ti.kind = TypeKind::Fn;
    ti.fn.params = param_copy;
    ti.fn.param_count = static_cast<uint32_t>(params.size());
    ti.fn.return_type = ret;
    ti.fn.effects = effects;
    return add(ti);
}

TypeId TypeTable::makeStruct(std::string_view name, std::span<const FieldInfo> fields,
                             bool is_packed, uint32_t explicit_align,
                             bool is_repr_c) {
    auto* field_copy = arena_.makeArray<FieldInfo>(fields.size());
    uint32_t offset = 0;
    uint32_t max_align = 1;
    uint32_t bit_pos = 0;       // current bit position within storage unit
    uint32_t unit_bits = 0;     // total bits in current storage unit (0 = none)

    for (size_t i = 0; i < fields.size(); ++i) {
        field_copy[i] = fields[i];
        uint32_t field_size = sizeOf(fields[i].type);
        uint32_t field_bits = field_size * 8;
        uint32_t field_align = is_packed ? 1 : alignOf(fields[i].type);

        if (fields[i].bit_width > 0) {
            // Bitfield: pack into current storage unit if same type and room exists
            if (unit_bits > 0 && bit_pos + fields[i].bit_width <= unit_bits) {
                // Pack into existing unit at current bit position
                field_copy[i].offset = static_cast<int32_t>(offset - field_size);
                field_copy[i].bit_offset = bit_pos;
                field_copy[i].bit_width = fields[i].bit_width;
                bit_pos += fields[i].bit_width;
            } else {
                // Start a new storage unit
                offset = (offset + field_align - 1) & ~(field_align - 1);
                field_copy[i].offset = static_cast<int32_t>(offset);
                field_copy[i].bit_offset = 0;
                field_copy[i].bit_width = fields[i].bit_width;
                bit_pos = fields[i].bit_width;
                unit_bits = field_bits;
                offset += field_size;
                if (field_align > max_align) max_align = field_align;
            }
        } else {
            // Regular field: reset bitfield packing state
            bit_pos = 0;
            unit_bits = 0;
            offset = (offset + field_align - 1) & ~(field_align - 1);
            field_copy[i].offset = static_cast<int32_t>(offset);
            offset += field_size;
            if (field_align > max_align) max_align = field_align;
        }
    }
    // Override alignment if explicit
    if (explicit_align > 0) max_align = explicit_align;
    // Pad total size to alignment
    uint32_t total_size = (offset + max_align - 1) & ~(max_align - 1);

    TypeInfo ti{};
    ti.kind = TypeKind::Struct;
    ti.struct_.name = name;
    ti.struct_.fields = field_copy;
    ti.struct_.field_count = static_cast<uint32_t>(fields.size());
    ti.struct_.size = total_size;
    ti.struct_.align = max_align;
    ti.struct_.is_packed = is_packed;
    ti.struct_.is_repr_c = is_repr_c;
    return add(ti);
}

TypeId TypeTable::makeOpaqueStruct(std::string_view name) {
    TypeInfo ti{};
    ti.kind = TypeKind::Struct;
    ti.struct_.name = name;
    ti.struct_.fields = nullptr;
    ti.struct_.field_count = 0;
    ti.struct_.size = 0;
    ti.struct_.align = 1;
    ti.struct_.is_packed = false;
    ti.struct_.is_repr_c = false;
    return add(ti);
}

void TypeTable::updateStruct(TypeId id, std::span<const FieldInfo> fields,
                             bool is_packed, uint32_t explicit_align,
                             bool is_repr_c) {
    assert(id < types_.size() && "TypeId out of range");
    auto* info = types_[id];
    assert(info->kind == TypeKind::Struct && "updateStruct on non-struct type");

    auto* field_copy = arena_.makeArray<FieldInfo>(fields.size());
    uint32_t offset = 0;
    uint32_t max_align = 1;
    uint32_t bit_pos = 0;
    uint32_t unit_bits = 0;

    for (size_t i = 0; i < fields.size(); ++i) {
        field_copy[i] = fields[i];
        uint32_t field_size = sizeOf(fields[i].type);
        uint32_t field_bits = field_size * 8;
        uint32_t field_align = is_packed ? 1 : alignOf(fields[i].type);

        if (fields[i].bit_width > 0) {
            if (unit_bits > 0 && bit_pos + fields[i].bit_width <= unit_bits) {
                field_copy[i].offset = static_cast<int32_t>(offset - field_size);
                field_copy[i].bit_offset = bit_pos;
                field_copy[i].bit_width = fields[i].bit_width;
                bit_pos += fields[i].bit_width;
            } else {
                offset = (offset + field_align - 1) & ~(field_align - 1);
                field_copy[i].offset = static_cast<int32_t>(offset);
                field_copy[i].bit_offset = 0;
                field_copy[i].bit_width = fields[i].bit_width;
                bit_pos = fields[i].bit_width;
                unit_bits = field_bits;
                offset += field_size;
                if (field_align > max_align) max_align = field_align;
            }
        } else {
            bit_pos = 0;
            unit_bits = 0;
            offset = (offset + field_align - 1) & ~(field_align - 1);
            field_copy[i].offset = static_cast<int32_t>(offset);
            offset += field_size;
            if (field_align > max_align) max_align = field_align;
        }
    }
    if (explicit_align > 0) max_align = explicit_align;
    uint32_t total_size = (offset + max_align - 1) & ~(max_align - 1);

    info->struct_.fields = field_copy;
    info->struct_.field_count = static_cast<uint32_t>(fields.size());
    info->struct_.size = total_size;
    info->struct_.align = max_align;
    info->struct_.is_packed = is_packed;
    info->struct_.is_repr_c = is_repr_c;
}

TypeId TypeTable::makeEnum(std::string_view name,
                           std::span<const std::string_view> variant_names,
                           std::span<const int64_t> values,
                           uint8_t backing_size) {
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
    ti.enum_.backing_size = backing_size;
    return add(ti);
}

TypeId TypeTable::makeUnion(std::string_view name, std::span<const VariantInfo> variants,
                            bool is_repr_c, uint8_t tag_size) {
    auto count = static_cast<uint32_t>(variants.size());
    auto* var_copy = arena_.makeArray<VariantInfo>(count);
    std::memcpy(var_copy, variants.data(), count * sizeof(VariantInfo));

    TypeInfo ti{};
    ti.kind = TypeKind::Union;
    ti.union_.name = name;
    ti.union_.variants = var_copy;
    ti.union_.variant_count = count;
    ti.union_.is_repr_c = is_repr_c;
    ti.union_.tag_size = tag_size;
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

TypeId TypeTable::makeDynTrait(std::string_view trait_name,
                               std::span<const std::string_view> method_names) {
    // Deduplicate by trait name
    for (size_t i = 0; i < types_.size(); ++i) {
        const auto& t = *types_[i];
        if (t.kind == TypeKind::DynTrait && t.dyn_trait.trait_name == trait_name) {
            return static_cast<TypeId>(i);
        }
    }
    TypeInfo ti{};
    ti.kind = TypeKind::DynTrait;
    ti.dyn_trait.trait_name = trait_name;
    ti.dyn_trait.method_count = static_cast<uint32_t>(method_names.size());
    if (!method_names.empty()) {
        ti.dyn_trait.method_names = arena_.makeArray<std::string_view>(method_names.size());
        for (size_t i = 0; i < method_names.size(); ++i) {
            ti.dyn_trait.method_names[i] = method_names[i];
        }
    } else {
        ti.dyn_trait.method_names = nullptr;
    }
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
        case PrimitiveKind::I64: case PrimitiveKind::U64: case PrimitiveKind::F64:
        case PrimitiveKind::Isize: case PrimitiveKind::Usize: return 8;  // 64-bit on x86-64
        case PrimitiveKind::Unit: return 0;
        case PrimitiveKind::Error: return 0;
        }
        return 0;
    case TypeKind::Struct:
        return ti.struct_.size;
    case TypeKind::Enum:
        return ti.enum_.backing_size;  // default 8, @repr(u8)=1, etc.
    case TypeKind::Union: {
        uint32_t max_payload = 0;
        for (uint32_t i = 0; i < ti.union_.variant_count; ++i) {
            if (ti.union_.variants[i].payload_type != INVALID_TYPE) {
                uint32_t ps = sizeOf(ti.union_.variants[i].payload_type);
                if (ps > max_payload) max_payload = ps;
            }
        }
        if (ti.union_.is_repr_c) {
            // Untagged union: no discriminant, all variants at offset 0
            return (max_payload + 7) & ~7u;
        }
        // Tagged union: tag + max payload, payload offset at 8 for alignment
        uint32_t tag_sz = ti.union_.tag_size;
        uint32_t payload_offset = (tag_sz + 7) & ~7u;  // round tag up to 8
        return payload_offset + ((max_payload + 7) & ~7u);
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
    case TypeKind::DynTrait:
        return 16; // fat pointer: data_ptr(8) + vtable_ptr(8)
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
        case PrimitiveKind::I64: case PrimitiveKind::U64: case PrimitiveKind::F64:
        case PrimitiveKind::Isize: case PrimitiveKind::Usize: return 8;
        case PrimitiveKind::Unit: case PrimitiveKind::Error: return 1;
        }
        return 1;
    case TypeKind::Struct:
        return ti.struct_.align;
    case TypeKind::Enum:
        return ti.enum_.backing_size;
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
    case TypeKind::DynTrait:
        return 8;
    }
    return 1;
}

int32_t TypeTable::offsetOf(TypeId id, std::string_view field_name) const {
    const auto& ti = get(id);
    if (ti.kind != TypeKind::Struct) return -1;
    for (uint32_t i = 0; i < ti.struct_.field_count; ++i) {
        if (ti.struct_.fields[i].name == field_name) {
            return ti.struct_.fields[i].offset;
        }
    }
    return -1;
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
    case PrimitiveKind::Isize:
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
    case PrimitiveKind::Isize: case PrimitiveKind::Usize:
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
        case PrimitiveKind::Isize: return "isize";
        case PrimitiveKind::Usize: return "usize";
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
    case TypeKind::DynTrait: return "dyn";
    }
    return "?";
}

std::string TypeTable::mangleName(TypeId id) const {
    if (id >= types_.size()) return "?";
    const auto& ti = *types_[id];
    switch (ti.kind) {
    case TypeKind::Primitive:
        return name(id);
    case TypeKind::Struct:
        return std::string(ti.struct_.name);
    case TypeKind::Enum:
        return std::string(ti.enum_.name);
    case TypeKind::Union:
        return std::string(ti.union_.name);
    case TypeKind::Ptr:
        return "Ptr_" + mangleName(ti.ptr.pointee);
    case TypeKind::PtrMut:
        return "PtrVar_" + mangleName(ti.ptr.pointee);
    case TypeKind::Array:
        return "Array_" + mangleName(ti.array.element) + "_" + std::to_string(ti.array.count);
    case TypeKind::Fn:
        return "Fn";
    case TypeKind::TypeVar:
        return std::string(ti.type_var.name);
    case TypeKind::Never:
        return "never";
    case TypeKind::DynTrait:
        return "dyn_" + std::string(ti.dyn_trait.trait_name);
    }
    return "?";
}

TypeTable::IntRange TypeTable::intRange(TypeId id) const {
    if (id >= types_.size()) return {0, 0};
    auto& ti = *types_[id];
    if (ti.kind != TypeKind::Primitive) return {0, 0};
    switch (ti.primitive.prim) {
        case PrimitiveKind::I8:  return {-128, 127};
        case PrimitiveKind::I16: return {-32768, 32767};
        case PrimitiveKind::I32: return {-2147483648LL, 2147483647};
        case PrimitiveKind::I64: return {INT64_MIN, static_cast<uint64_t>(INT64_MAX)};
        case PrimitiveKind::U8:  return {0, 255};
        case PrimitiveKind::U16: return {0, 65535};
        case PrimitiveKind::U32: return {0, 4294967295ULL};
        case PrimitiveKind::U64: return {0, UINT64_MAX};
        default: return {0, 0};
    }
}

} // namespace kern
