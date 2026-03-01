#pragma once
#include "kern/hir/HIR.h"
#include "kern/parser/AST.h"
#include "kern/support/CompilationContext.h"
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace kern {

class HIRBuilder {
public:
    explicit HIRBuilder(CompilationContext& ctx);

    HIRModule* build(const Module* ast);

    bool hasErrors() const;

private:
    // Type resolution: AST TypeRef → TypeId
    TypeId resolveType(const TypeRef& ref);
    TypeId resolvePointee(const TypeRef& ref);

    // Declaration processing
    void registerStructDecls(const Module* ast);
    void registerEnumDecls(const Module* ast);
    void registerUnionDecls(const Module* ast);
    void registerFnSigs(const Module* ast);

    // Function building
    HIRFnDecl* buildFn(const FnDecl* fn);

    // Expression building (type check + HIR generation)
    HIRExpr* buildExpr(const Expr* expr, std::optional<TypeId> ctx_type = std::nullopt);
    HIRExpr* buildIntLit(const Expr* expr, std::optional<TypeId> ctx_type);
    HIRExpr* buildFloatLit(const Expr* expr, std::optional<TypeId> ctx_type);
    HIRExpr* buildBoolLit(const Expr* expr);
    HIRExpr* buildStringLit(const Expr* expr);
    HIRExpr* buildIdent(const Expr* expr);
    HIRExpr* buildBinOp(const Expr* expr, std::optional<TypeId> ctx_type);
    HIRExpr* buildUnaryOp(const Expr* expr);
    HIRExpr* buildCall(const Expr* expr);
    HIRExpr* buildIf(const Expr* expr, std::optional<TypeId> ctx_type);
    HIRExpr* buildMatch(const Expr* expr, std::optional<TypeId> ctx_type);
    HIRExpr* buildBlock(const Expr* expr, std::optional<TypeId> ctx_type);
    HIRExpr* buildReturn(const Expr* expr);
    HIRExpr* buildStructLit(const Expr* expr);
    HIRExpr* buildFieldAccess(const Expr* expr);
    HIRExpr* buildEnumAccess(const Expr* expr);
    HIRExpr* buildUnionVariant(const Expr* expr);
    HIRExpr* buildCast(const Expr* expr);
    HIRExpr* buildLoop(const Expr* expr, std::optional<TypeId> ctx_type);
    HIRExpr* buildArrayLit(const Expr* expr);
    HIRExpr* buildIndexAccess(const Expr* expr);
    HIRExpr* buildInlineAsm(const Expr* expr);
    HIRExpr* buildLambda(const Expr* expr, std::optional<TypeId> ctx_type);
    HIRExpr* buildMethodCall(const Expr* expr);

    // Statement building
    HIRStmt* buildStmt(const Stmt* stmt);

    // Pattern building
    HIRPattern* buildPattern(const Pattern* pat, TypeId scrutinee_type);

    // Error helpers
    HIRExpr* errorExpr(SourceLocation loc);

    // Type query helpers
    bool isIntegerType(TypeId id) const;
    bool isFloatType(TypeId id) const;
    bool isSignedType(TypeId id) const;
    bool intFitsInType(int64_t value, TypeId type) const;

    CompilationContext& ctx_;

    // Function signatures
    struct FnSig {
        std::string_view name;
        std::vector<TypeId> param_types;
        TypeId return_type;
    };
    std::unordered_map<std::string_view, FnSig> fn_table_;

    // Local scope
    std::unordered_map<std::string_view, TypeId> local_vars_;
    std::unordered_set<std::string_view> mutable_vars_;
    TypeId current_return_type_ = INVALID_TYPE;

    // Type name → TypeId for struct/enum/union
    std::unordered_map<std::string_view, TypeId> named_types_;

    // Generic struct/union templates (for on-demand monomorphization)
    std::unordered_map<std::string_view, const StructDecl*> generic_structs_;
    std::unordered_map<std::string_view, const UnionDecl*> generic_unions_;

    // Lambda lifting: accumulated lifted functions to add to module
    std::vector<HIRFnDecl*> lifted_lambdas_;
    uint32_t lambda_counter_ = 0;

    // Trait/impl support (static dispatch)
    struct TraitInfo {
        std::string_view name;
        std::vector<std::string_view> method_names;
    };
    std::unordered_map<std::string_view, TraitInfo> trait_table_;

    // Map: type_name → { method_name → mangled_fn_name }
    struct ImplMethods {
        std::unordered_map<std::string_view, std::string_view> methods;
    };
    std::unordered_map<std::string_view, ImplMethods> impl_table_;

    void registerTraits(const Module* ast);
    void registerImpls(const Module* ast);

    // Resolve method on a type: returns mangled fn name or empty
    std::string_view resolveMethod(TypeId type, std::string_view method) const;
};

} // namespace kern
