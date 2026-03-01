#pragma once
#include "kern/support/Arena.h"
#include <cstdint>
#include <string_view>
#include <vector>
#include <span>

namespace kern {

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
};

struct FieldInfo {
    std::string_view name;
    TypeId type;
    bool is_mutable;
    int32_t offset;  // byte offset within struct, -1 if not yet computed
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
};

struct EnumData {
    std::string_view name;
    int64_t* values;
    std::string_view* names;
    uint32_t variant_count;
};

struct UnionData {
    std::string_view name;
    VariantInfo* variants;
    uint32_t variant_count;
};

struct PtrData {
    TypeId pointee;
    bool is_mutable;
};

struct FnData {
    TypeId* params;
    uint32_t param_count;
    TypeId return_type;
};

struct ArrayData {
    TypeId element;
    uint32_t count;
};

struct TypeVarData {
    std::string_view name;
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
    static constexpr TypeId PRIMITIVE_COUNT = 13;

    // Register a new type and return its TypeId.
    TypeId add(TypeInfo info);

    // Look up a type by id.
    const TypeInfo& get(TypeId id) const;

    // Number of registered types (including primitives).
    size_t size() const { return types_.size(); }

    // Convenience constructors — return TypeId
    TypeId makePtr(TypeId pointee, bool is_mutable);
    TypeId makeFn(std::span<const TypeId> params, TypeId ret);
    TypeId makeStruct(std::string_view name, std::span<const FieldInfo> fields);
    TypeId makeEnum(std::string_view name, std::span<const std::string_view> variant_names,
                    std::span<const int64_t> values);
    TypeId makeUnion(std::string_view name, std::span<const VariantInfo> variants);

    // Type queries
    uint32_t sizeOf(TypeId id) const;
    uint32_t alignOf(TypeId id) const;
    uint32_t bitWidth(TypeId id) const;
    bool isFloat(TypeId id) const;
    bool isSigned(TypeId id) const;
    bool isInteger(TypeId id) const;
    bool isPrimitive(TypeId id) const;
    const char* name(TypeId id) const;

private:
    void registerPrimitives();

    Arena& arena_;
    std::vector<TypeInfo*> types_;
};

} // namespace kern
