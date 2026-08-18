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

    // Register types and function signatures from an external module
    // without building HIR. Used for cross-module import injection.
    // module_path is the module's dotted path (e.g. "math", "kern.memory").
    void registerExports(const Module* ast, std::string_view module_path = {});

    // Inject a single function signature from an external module
    void injectFnSig(std::string_view name, const std::vector<TypeId>& param_types,
                     TypeId return_type);

    // Inject a named type from an external module
    void injectNamedType(std::string_view name, TypeId tid);

    // Inject a generic struct template from an external module
    void injectGenericStruct(std::string_view name, const StructDecl* decl);

    // Inject a generic union template from an external module
    void injectGenericUnion(std::string_view name, const UnionDecl* decl);

    // Inject a global variable type from an external module
    void injectGlobalType(std::string_view name, TypeId tid);

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
    HIRExpr* buildCStringLit(const Expr* expr);
    HIRExpr* buildStringInterp(const Expr* expr);
    HIRExpr* buildIdent(const Expr* expr);
    HIRExpr* buildBinOp(const Expr* expr, std::optional<TypeId> ctx_type);
    HIRExpr* buildUnaryOp(const Expr* expr);
    HIRExpr* buildCall(const Expr* expr);
    HIRExpr* buildIf(const Expr* expr, std::optional<TypeId> ctx_type);
    HIRExpr* buildMatch(const Expr* expr, std::optional<TypeId> ctx_type);
    HIRExpr* buildBlock(const Expr* expr, std::optional<TypeId> ctx_type);
    HIRExpr* buildReturn(const Expr* expr);
    HIRExpr* buildStructLit(const Expr* expr, std::optional<TypeId> ctx_type = std::nullopt);
    HIRExpr* buildFieldAccess(const Expr* expr);
    HIRExpr* buildEnumAccess(const Expr* expr);
    HIRExpr* buildUnionVariant(const Expr* expr,
                               std::optional<TypeId> ctx_type = std::nullopt);
    HIRExpr* buildCast(const Expr* expr);
    HIRExpr* buildLoop(const Expr* expr, std::optional<TypeId> ctx_type);
    HIRExpr* buildForRange(const Expr* expr);
    HIRExpr* buildForEach(const Expr* expr);
    HIRExpr* buildWhileLoop(const Expr* expr);
    HIRExpr* buildArrayLit(const Expr* expr);
    HIRExpr* buildArrayRepeat(const Expr* expr);
    HIRExpr* buildIndexAccess(const Expr* expr);
    HIRExpr* buildSliceExpr(const Expr* expr);
    HIRExpr* buildInlineAsm(const Expr* expr);
    HIRExpr* buildLambda(const Expr* expr, std::optional<TypeId> ctx_type);
    HIRExpr* buildMethodCall(const Expr* expr);
    HIRExpr* buildTry(const Expr* expr);
    HIRExpr* buildTupleLit(const Expr* expr);

    // Statement building
    HIRStmt* buildStmt(const Stmt* stmt);
    void buildTupleDestructStmts(const Stmt* stmt, HIRStmt** out, uint32_t& idx);

    // Pattern building
    HIRPattern* buildPattern(const Pattern* pat, TypeId scrutinee_type);

    // Error helpers
    HIRExpr* errorExpr(SourceLocation loc);

    // Implicit integer widening: wraps expr in a Cast if it can be widened to target
    HIRExpr* implicitWiden(HIRExpr* expr, TypeId target);

    // Type query helpers
    bool isIntegerType(TypeId id) const;
    bool isFloatType(TypeId id) const;
    bool isSignedType(TypeId id) const;
    bool intFitsInType(int64_t value, TypeId type) const;

    // Compile-time evaluation for const fn / static_assert
    bool constEvalInt(HIRExpr* expr, int64_t* out,
                      const std::unordered_map<std::string_view, int64_t>* env = nullptr);
    bool constEvalFloat(HIRExpr* expr, double* out,
                        const std::unordered_map<std::string_view, int64_t>* env = nullptr);

    // Fold constant expressions in global initializers to literal nodes
    HIRExpr* constFoldExpr(HIRExpr* expr);

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
    std::string_view current_fn_name_;  // for generic bounds lookup

    // Type name → TypeId for struct/enum/union
    std::unordered_map<std::string_view, TypeId> named_types_;

    // Generic struct/union/type-alias/fn templates (for on-demand monomorphization)
    std::unordered_map<std::string_view, const StructDecl*> generic_structs_;
    std::unordered_map<std::string_view, const UnionDecl*> generic_unions_;
    std::unordered_map<std::string_view, const TypeAliasDecl*> generic_type_aliases_;
    std::unordered_map<std::string_view, const FnDecl*> generic_extern_fns_;  // cross-module generic bodies

    // Const generic parameter values (name → value), scoped during instantiation
    std::unordered_map<std::string_view, int64_t> const_values_;

    // Lambda lifting: accumulated lifted functions to add to module
    std::vector<HIRFnDecl*> lifted_lambdas_;
    uint32_t lambda_counter_ = 0;

    // Closure capture: maps lifted lambda name → captured variable info
    struct CapturedVar {
        std::string_view name;  // interned
        TypeId type;
        bool is_mutable;
    };
    std::unordered_map<std::string_view, std::vector<CapturedVar>> lambda_captures_;

    // Per-lambda capture tracking state
    bool in_lambda_ = false;
    std::unordered_map<std::string_view, TypeId> outer_locals_;   // outer scope vars
    std::unordered_set<std::string_view> outer_mutables_;         // outer scope mutables
    std::vector<CapturedVar> current_captures_;                    // captures found in current lambda

    // Track which local variables hold which lambda (for capture arg injection)
    std::unordered_map<std::string_view, std::string_view> local_lambda_map_;  // local_var_name → lambda_fn_name

    // Closure struct types: TypeIds of synthetic structs created for capturing closures
    std::unordered_set<TypeId> closure_struct_types_;

    // Map closure struct TypeId → lifted lambda function name
    std::unordered_map<TypeId, std::string_view> closure_fn_names_;

    // Trait/impl support (static dispatch)
    struct TraitInfo {
        std::string_view name;
        std::vector<std::string_view> method_names;
        std::vector<EffectSet> method_effects;  // declared effects per method
        std::vector<std::string_view> assoc_type_names;  // associated type names
        const TraitDecl* ast = nullptr;  // for method sig lookups (TypeVar dispatch)
    };
    std::unordered_map<std::string_view, TraitInfo> trait_table_;

    // Map: type_name → { method_name → mangled_fn_name }
    struct ImplMethods {
        std::unordered_map<std::string_view, std::string_view> methods;
    };
    std::unordered_map<std::string_view, ImplMethods> impl_table_;

    // Associated type resolution: "TypeName::AssocTypeName" → TypeId
    std::unordered_map<std::string_view, TypeId> assoc_type_map_;

    // Current impl target type name (for Self:: resolution in method bodies)
    std::string_view current_impl_type_name_;

    void registerTraits(const Module* ast);
    void registerImpls(const Module* ast);
    bool typeImplementsTrait(std::string_view type_name, std::string_view trait_name);

    // Generic function type param bounds: fn_name → list of (type_var_id, bounds[])
    struct GenericBound {
        TypeId type_var_id;
        std::string_view param_name;
        std::string_view* bounds;
        uint32_t bound_count;
    };
    std::unordered_map<std::string_view, std::vector<GenericBound>> generic_bounds_;

    // Resolve method on a type: returns mangled fn name or empty
    std::string_view resolveMethod(TypeId type, std::string_view method) const;

    // dyn Trait vtable tracking: key = "TypeName_TraitName", value = vtable label
    struct VTableInfo {
        std::string_view label;         // interned vtable global label
        std::string_view trait_name;    // trait name
        std::string_view type_name;     // concrete type name
        std::vector<std::string_view> fn_labels;  // ordered function labels for vtable slots
        uint32_t self_size = 0;         // byte size of concrete implementing type
    };
    std::vector<VTableInfo> vtable_globals_;

    // Built HIR functions (for const fn evaluation)
    std::unordered_map<std::string_view, HIRFnDecl*> hir_fns_;

    // Global variables (static val/var)
    std::unordered_map<std::string_view, HIRGlobalDecl*> global_vars_;
    std::unordered_map<std::string_view, TypeId> global_types_;  // pre-registered for ident resolution

    // Cross-module import tracking: fn_name → source module path
    std::unordered_map<std::string_view, std::string_view> fn_module_map_;

    // Cross-module visibility: struct_name → defining module path
    std::unordered_map<std::string_view, std::string_view> struct_module_map_;
    // Current module being compiled (empty for single-file compilation)
    std::string_view current_module_;

    // Drop trait support: type_name → mangled drop fn name
    std::unordered_map<std::string_view, std::string_view> drop_fns_;

    // Copy/Clone trait support
    std::unordered_set<std::string_view> copy_types_;   // types implementing Copy trait
    std::unordered_set<std::string_view> clone_types_;   // types implementing Clone trait

    // Deref/DerefMut trait support: type_name → deref target TypeId
    std::unordered_map<std::string_view, TypeId> deref_targets_;
    std::unordered_map<std::string_view, TypeId> deref_mut_targets_;

    // Send/Sync marker traits: types explicitly implementing or opting out
    std::unordered_set<std::string_view> send_types_;
    std::unordered_set<std::string_view> not_send_types_;
    std::unordered_set<std::string_view> sync_types_;
    std::unordered_set<std::string_view> not_sync_types_;
    bool isSendType(TypeId type) const;
    bool isSyncType(TypeId type) const;

    // Check if a type is Copy (primitive or explicitly marked)
    bool isCopyType(TypeId type) const;

    // Track local variable declaration order within current scope for drop ordering
    struct ScopeVar {
        std::string_view name;
        TypeId type;
    };
    std::vector<std::vector<ScopeVar>> scope_stack_;  // stack of scope frames

    // Check if a type needs dropping (implements Drop trait)
    bool needsDrop(TypeId type) const;

    // Generate a drop call HIRExpr for a variable
    HIRExpr* makeDropCall(std::string_view var_name, TypeId type, SourceLocation loc);

    // Push/pop scope frames
    void pushScope();
    void popScope();

    // Record a local variable in the current scope frame
    void recordScopeVar(std::string_view name, TypeId type);
};

} // namespace kern
