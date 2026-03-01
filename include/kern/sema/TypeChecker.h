#pragma once
#include "kern/parser/AST.h"
#include "kern/support/Arena.h"
#include "kern/support/Diagnostic.h"
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace kern {

enum class Type {
    I8, I16, I32, I64,
    U8, U16, U32, U64,
    F32, F64,
    Bool, Unit, Error,
    Struct, Enum, Union
};

const char* typeName(Type t);
bool isInteger(Type t);
bool isSigned(Type t);
bool isUnsigned(Type t);
int bitWidth(Type t);
bool isFloat(Type t);
bool intFitsInType(int64_t value, Type t);
bool isStruct(Type t);
bool isEnum(Type t);
bool isUnion(Type t);
int sizeBytes(Type t);

struct FieldDef {
    std::string_view name;
    Type type;
    bool is_mutable;
    int32_t offset;   // byte offset within struct
    std::string_view struct_name; // if type==Struct, the nested struct name
};

struct StructDef {
    std::string_view name;
    std::vector<FieldDef> fields;
    int32_t total_size;
    int32_t alignment;
};

struct EnumVariantDef {
    std::string_view name;
    int32_t tag;  // ordinal (0, 1, 2, ...)
};

struct EnumDef {
    std::string_view name;
    std::vector<EnumVariantDef> variants;
    int32_t tag_size;  // 1 (u8), 2 (u16), or 4 (u32)
};

struct UnionVariantDef {
    std::string_view name;
    int32_t tag;
    Type payload_type;               // Error = no payload
    std::string_view payload_struct_name; // if payload_type == Struct
    int32_t payload_size;            // 0 for empty variants
};

struct UnionDef {
    std::string_view name;
    std::vector<UnionVariantDef> variants;
    int32_t tag_size;
    int32_t max_payload_size;
    int32_t total_size;              // tag_size + padding + max_payload_size
    int32_t alignment;
};

struct FnSig {
    std::string_view name;
    std::vector<Type> param_types;
    std::vector<std::string_view> param_struct_names; // parallel to param_types
    Type return_type;
    std::string_view return_struct_name; // when return_type == Type::Struct
};

class TypeChecker {
public:
    TypeChecker(DiagnosticEngine& diag, Arena* arena = nullptr);

    bool check(Module* mod);

    // Query the type of an expression (after check() has run)
    Type typeOfExpr(const Expr* expr) const;

    // Struct queries (after check() has run)
    const StructDef* getStructDef(std::string_view name) const;
    std::string_view structNameOfExpr(const Expr* expr) const;
    std::string_view localStructName(std::string_view binding) const;

    // Enum/Union queries (after check() has run)
    const EnumDef* getEnumDef(std::string_view name) const;
    const UnionDef* getUnionDef(std::string_view name) const;
    std::string_view enumNameOfExpr(const Expr* expr) const;
    std::string_view unionNameOfExpr(const Expr* expr) const;
    std::string_view localEnumName(std::string_view binding) const;
    std::string_view localUnionName(std::string_view binding) const;

private:
    Type checkFn(FnDecl* fn);
    Type checkExpr(Expr* expr, std::optional<Type> ctx = std::nullopt);
    Type checkBlock(BlockExpr* block, std::optional<Type> ctx = std::nullopt);
    void checkStmt(Stmt* stmt);

    Type resolveType(const TypeRef& ref);

    DiagnosticEngine& diag_;
    Arena* arena_ = nullptr;
    std::unordered_map<std::string_view, FnSig> fn_table_;
    std::unordered_map<std::string_view, Type> local_vars_;
    std::unordered_set<std::string_view> mutable_vars_;
    Type current_return_type_ = Type::Error;

    // Memoized expression types (Expr* -> Type)
    std::unordered_map<const Expr*, Type> expr_types_;

    // Struct support
    std::unordered_map<std::string_view, StructDef> struct_defs_;
    std::unordered_map<const Expr*, std::string_view> expr_struct_names_;
    std::unordered_map<std::string_view, std::string_view> local_struct_names_;

    // Enum support
    std::unordered_map<std::string_view, EnumDef> enum_defs_;
    std::unordered_map<const Expr*, std::string_view> expr_enum_names_;
    std::unordered_map<std::string_view, std::string_view> local_enum_names_;

    // Union support
    std::unordered_map<std::string_view, UnionDef> union_defs_;
    std::unordered_map<const Expr*, std::string_view> expr_union_names_;
    std::unordered_map<std::string_view, std::string_view> local_union_names_;
};

} // namespace kern
