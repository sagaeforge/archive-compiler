#pragma once
#include "kern/hir/HIR.h"
#include "kern/support/CompilationContext.h"
#include <unordered_map>
#include <vector>
#include <string>

namespace kern {

class MonomorphizationPass {
public:
    explicit MonomorphizationPass(CompilationContext& ctx) : ctx_(ctx) {}

    // Run monomorphization: clone generic functions for each concrete type instantiation.
    // Returns a new HIRModule with generic functions replaced by specialized copies.
    HIRModule* run(HIRModule* mod);

private:
    CompilationContext& ctx_;

    // Map from generic fn name → HIRFnDecl*
    std::unordered_map<std::string_view, HIRFnDecl*> generic_fns_;

    // Map from mangled name → already-created specialization
    std::unordered_map<std::string, HIRFnDecl*> specializations_;

    // Collect all instantiation sites: (generic_name, [concrete type args])
    struct Instantiation {
        std::string_view generic_name;
        std::vector<TypeId> type_args;
        std::string mangled_name;
    };
    std::vector<Instantiation> instantiations_;

    // Phase 1: Collect generic functions and their call sites
    void collectGenericFns(HIRModule* mod);
    void collectInstantiations(HIRExpr* expr);

    // Phase 2: Create specialized copies
    HIRFnDecl* specialize(HIRFnDecl* generic, const std::vector<TypeId>& type_args,
                          std::string_view mangled_name);

    // Deep copy + type substitution
    HIRExpr* cloneExpr(HIRExpr* expr, const std::unordered_map<TypeId, TypeId>& subst);
    HIRStmt* cloneStmt(HIRStmt* stmt, const std::unordered_map<TypeId, TypeId>& subst);
    HIRPattern* clonePattern(HIRPattern* pat, const std::unordered_map<TypeId, TypeId>& subst);
    TypeId substituteType(TypeId type, const std::unordered_map<TypeId, TypeId>& subst);

    // Phase 3: Patch call sites to use mangled names
    void patchCallSites(HIRExpr* expr);
    void patchCallSitesInStmt(HIRStmt* stmt);

    // Type inference: given a generic fn signature and concrete arg types, infer type args
    std::vector<TypeId> inferTypeArgs(HIRFnDecl* generic, const std::vector<TypeId>& arg_types);

    // Mangle name: identity + [i64] → "identity_i64"
    std::string mangleName(std::string_view base, const std::vector<TypeId>& type_args);
};

} // namespace kern
