#pragma once
#include "kern/support/Arena.h"
#include <cstdint>
#include <string_view>
#include <vector>
#include <span>

namespace kern {

// ============================================================================
// Effect System — compile-time annotations erased before LIR
// ============================================================================

enum class Effect : uint8_t {
    Mut    = 1 << 0,   // Mutable state (var bindings, field mutation)
    Mem    = 1 << 1,   // Pointer/heap memory access
    IO     = 1 << 2,   // Hardware I/O, volatile, asm, intrinsics
    Atomic = 1 << 3,   // Atomic operations (lock cmpxchg, lock xadd, fences)
};

using EffectSet = uint8_t;

static constexpr EffectSet EFFECT_NONE   = 0;
static constexpr EffectSet EFFECT_MUT    = static_cast<EffectSet>(Effect::Mut);
static constexpr EffectSet EFFECT_MEM    = static_cast<EffectSet>(Effect::Mem);
static constexpr EffectSet EFFECT_IO     = static_cast<EffectSet>(Effect::IO);
static constexpr EffectSet EFFECT_ATOMIC = static_cast<EffectSet>(Effect::Atomic);
static constexpr EffectSet EFFECT_ALL   = EFFECT_MUT | EFFECT_MEM | EFFECT_IO | EFFECT_ATOMIC;

inline bool hasEffect(EffectSet set, Effect e) {
    return (set & static_cast<uint8_t>(e)) != 0;
}

inline EffectSet addEffect(EffectSet set, Effect e) {
    return set | static_cast<uint8_t>(e);
}

inline EffectSet unionEffects(EffectSet a, EffectSet b) {
    return a | b;
}

// Returns true if 'sub' is a subset of 'super' (caller has all callee's effects)
inline bool effectSubset(EffectSet sub, EffectSet super) {
    return (sub & super) == sub;
}

// Parse effect name string to Effect enum (returns false if unknown)
inline bool parseEffectName(std::string_view name, Effect& out) {
    if (name == "mut")    { out = Effect::Mut;    return true; }
    if (name == "mem")    { out = Effect::Mem;    return true; }
    if (name == "io")     { out = Effect::IO;     return true; }
    if (name == "atomic") { out = Effect::Atomic; return true; }
    return false;
}

inline const char* effectName(Effect e) {
    switch (e) {
        case Effect::Mut:    return "mut";
        case Effect::Mem:    return "mem";
        case Effect::IO:     return "io";
        case Effect::Atomic: return "atomic";
    }
    return "?";
}

// Format an EffectSet as comma-separated names (e.g. "io, atomic")
// Returns "pure" for empty set
inline std::string effectSetString(EffectSet set) {
    if (set == EFFECT_NONE) return "pure";
    std::string result;
    static constexpr Effect ALL_EFFECTS[] = {Effect::Mut, Effect::Mem, Effect::IO, Effect::Atomic};
    for (auto e : ALL_EFFECTS) {
        if (hasEffect(set, e)) {
            if (!result.empty()) result += ", ";
            result += effectName(e);
        }
    }
    return result;
}

// ============================================================================
// Type System
// ============================================================================

// TypeId is a lightweight handle into the TypeTable.
// Primitives are pre-registered at indices 0-12.
using TypeId = uint32_t;

// Sentinel for invalid/unknown types.
static constexpr TypeId INVALID_TYPE = UINT32_MAX;

enum class PrimitiveKind : uint8_t {
    I8, I16, I32, I64,
    U8, U16, U32, U64,
    F32, F64,
    Bool, Unit, Error
};

enum class TypeKind : uint8_t {
    Primitive,
    Struct,
    Enum,
    Union,
    Ptr,
    PtrMut,
    Fn,
    Array,
    TypeVar,
    Never,  // bottom type (!)
    DynTrait,  // dyn Trait — fat pointer {data_ptr, vtable_ptr}
};

struct FieldInfo {
    std::string_view name;
    TypeId type;
    bool is_mutable;
    int32_t offset;         // byte offset within struct, -1 if not yet computed
    uint32_t bit_width = 0; // 0 = full-width field, >0 = bitfield width in bits
    uint32_t bit_offset = 0;// bit offset within the storage unit (for bitfields)
    bool is_pub = true;     // pub field — accessible from other modules (default: true)
};

struct VariantInfo {
    std::string_view name;
    TypeId payload_type;  // INVALID_TYPE if no payload
};

struct PrimitiveData {
    PrimitiveKind prim;
};

struct StructData {
    std::string_view name;
    FieldInfo* fields;
    uint32_t field_count;
    uint32_t size;
    uint32_t align;
    bool is_packed;
    bool is_repr_c;   // @repr(C) — guaranteed C ABI-compatible layout
};

struct EnumData {
    std::string_view name;
    int64_t* values;
    std::string_view* names;
    uint32_t variant_count;
    uint8_t backing_size = 8;  // 1/2/4/8 bytes (@repr(u8/u16/u32/u64))
};

struct UnionData {
    std::string_view name;
    VariantInfo* variants;
    uint32_t variant_count;
    bool is_repr_c = false;     // untagged C-style union (no discriminant tag)
};

struct PtrData {
    TypeId pointee;
    bool is_mutable;
};

struct FnData {
    TypeId* params;
    uint32_t param_count;
    TypeId return_type;
    EffectSet effects;     // declared effects (0 = pure)
};

struct ArrayData {
    TypeId element;
    uint32_t count;
};

struct TypeVarData {
    std::string_view name;
};

struct DynTraitData {
    std::string_view trait_name;    // interned trait name
    std::string_view* method_names; // ordered method names (vtable slot order)
    uint32_t method_count;
};

struct TypeInfo {
    TypeKind kind;
    union {
        PrimitiveData primitive;
        StructData struct_;
        EnumData enum_;
        UnionData union_;
        PtrData ptr;
        FnData fn;
        ArrayData array;
        TypeVarData type_var;
        DynTraitData dyn_trait;
    };

    static TypeInfo makePrimitive(PrimitiveKind p) {
        TypeInfo ti{};
        ti.kind = TypeKind::Primitive;
        ti.primitive.prim = p;
        return ti;
    }
};

// TypeTable: central registry of all types, shared across all pipeline stages.
// Lives in the Support layer so HIR, LIR, and Backend can all reference TypeId.
class TypeTable {
public:
    explicit TypeTable(Arena& arena);

    // Pre-registered primitive TypeIds
    static constexpr TypeId I8    = 0;
    static constexpr TypeId I16   = 1;
    static constexpr TypeId I32   = 2;
    static constexpr TypeId I64   = 3;
    static constexpr TypeId U8    = 4;
    static constexpr TypeId U16   = 5;
    static constexpr TypeId U32   = 6;
    static constexpr TypeId U64   = 7;
    static constexpr TypeId F32   = 8;
    static constexpr TypeId F64   = 9;
    static constexpr TypeId Bool  = 10;
    static constexpr TypeId Unit  = 11;
    static constexpr TypeId Error = 12;
    static constexpr TypeId Never = 13;  // bottom type (!)
    static constexpr TypeId PRIMITIVE_COUNT = 14;

    // Register a new type and return its TypeId.
    TypeId add(TypeInfo info);

    // Look up a type by id.
    const TypeInfo& get(TypeId id) const;

    // Number of registered types (including primitives).
    size_t size() const { return types_.size(); }

    // Convenience constructors — return TypeId
    TypeId makePtr(TypeId pointee, bool is_mutable);
    TypeId makeFn(std::span<const TypeId> params, TypeId ret, EffectSet effects = EFFECT_NONE);
    TypeId makeStruct(std::string_view name, std::span<const FieldInfo> fields,
                      bool is_packed = false, uint32_t explicit_align = 0,
                      bool is_repr_c = false);
    // Create a forward-declared struct (no fields/size yet) for self-referential types
    TypeId makeOpaqueStruct(std::string_view name);
    // Fill in fields for a previously opaque struct
    void updateStruct(TypeId id, std::span<const FieldInfo> fields,
                      bool is_packed = false, uint32_t explicit_align = 0,
                      bool is_repr_c = false);
    TypeId makeEnum(std::string_view name, std::span<const std::string_view> variant_names,
                    std::span<const int64_t> values, uint8_t backing_size = 8);
    TypeId makeUnion(std::string_view name, std::span<const VariantInfo> variants,
                     bool is_repr_c = false);
    TypeId makeArrayType(TypeId element, uint32_t count);
    TypeId makeDynTrait(std::string_view trait_name,
                        std::span<const std::string_view> method_names);

    // Type queries
    uint32_t sizeOf(TypeId id) const;
    uint32_t alignOf(TypeId id) const;
    int32_t offsetOf(TypeId id, std::string_view field_name) const;
    uint32_t bitWidth(TypeId id) const;
    bool isFloat(TypeId id) const;
    bool isSigned(TypeId id) const;
    bool isInteger(TypeId id) const;
    bool isPrimitive(TypeId id) const;
    const char* name(TypeId id) const;

    // Integer range: returns {min, max} for the given integer type.
    // Returns {0, 0} for non-integer types.
    struct IntRange { int64_t min; uint64_t max; };
    IntRange intRange(TypeId id) const;

private:
    void registerPrimitives();

    Arena& arena_;
    std::vector<TypeInfo*> types_;
};

} // namespace kern
