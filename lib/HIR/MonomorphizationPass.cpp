#include "kern/hir/MonomorphizationPass.h"
#include <cassert>

namespace kern {

// ============================================================================
// Entry point
// ============================================================================

HIRModule* MonomorphizationPass::run(HIRModule* mod) {
    if (!mod) return nullptr;

    // Phase 1: Identify generic functions
    collectGenericFns(mod);
    if (generic_fns_.empty()) return mod;  // nothing to monomorphize

    // Phase 1b: Collect all instantiation sites from non-generic function bodies
    for (uint32_t i = 0; i < mod->fn_count; ++i) {
        if (mod->functions[i]->type_param_count == 0 && mod->functions[i]->body) {
            collectInstantiations(mod->functions[i]->body);
        }
    }

    // Phase 2: Create specialized copies for each instantiation
    for (auto& inst : instantiations_) {
        auto it = generic_fns_.find(inst.generic_name);
        if (it == generic_fns_.end()) continue;
        if (specializations_.count(inst.mangled_name)) continue;  // already done

        auto* spec = specialize(it->second, inst.type_args, inst.mangled_name);
        specializations_[inst.mangled_name] = spec;
    }

    // Phase 3: Patch call sites in all non-generic functions
    for (uint32_t i = 0; i < mod->fn_count; ++i) {
        if (mod->functions[i]->type_param_count == 0 && mod->functions[i]->body) {
            patchCallSites(mod->functions[i]->body);
        }
    }

    // Build new module: non-generic fns + specialized fns
    std::vector<HIRFnDecl*> new_fns;
    for (uint32_t i = 0; i < mod->fn_count; ++i) {
        if (mod->functions[i]->type_param_count == 0) {
            new_fns.push_back(mod->functions[i]);
        }
    }
    for (auto& [name, spec] : specializations_) {
        new_fns.push_back(spec);
    }

    auto* new_mod = ctx_.arena.make<HIRModule>();
    new_mod->struct_count = mod->struct_count;
    new_mod->structs = mod->structs;
    new_mod->enum_count = mod->enum_count;
    new_mod->enums = mod->enums;
    new_mod->union_count = mod->union_count;
    new_mod->unions = mod->unions;
    new_mod->global_count = mod->global_count;
    new_mod->globals = mod->globals;
    new_mod->fn_count = static_cast<uint32_t>(new_fns.size());
    new_mod->functions = ctx_.arena.makeArray<HIRFnDecl*>(new_fns.size());
    for (size_t i = 0; i < new_fns.size(); ++i) {
        new_mod->functions[i] = new_fns[i];
    }

    return new_mod;
}

// ============================================================================
// Phase 1: Collect generic functions
// ============================================================================

void MonomorphizationPass::collectGenericFns(HIRModule* mod) {
    for (uint32_t i = 0; i < mod->fn_count; ++i) {
        if (mod->functions[i]->type_param_count > 0) {
            generic_fns_[mod->functions[i]->name] = mod->functions[i];
        }
    }
}

void MonomorphizationPass::collectInstantiations(HIRExpr* expr) {
    if (!expr) return;

    switch (expr->kind) {
        case HIRExpr::Kind::Call: {
            auto* call = static_cast<HIRCallExpr*>(expr);
            // Check if calling a generic function
            auto it = generic_fns_.find(call->callee);
            if (it != generic_fns_.end()) {
                // Infer type args from argument types
                std::vector<TypeId> arg_types;
                for (uint32_t i = 0; i < call->arg_count; ++i) {
                    arg_types.push_back(call->args[i]->type);
                }
                auto type_args = inferTypeArgs(it->second, arg_types);
                if (!type_args.empty()) {
                    Instantiation inst;
                    inst.generic_name = call->callee;
                    inst.type_args = type_args;
                    inst.mangled_name = mangleName(call->callee, type_args);
                    instantiations_.push_back(inst);
                }
            }
            // Recurse into arguments
            for (uint32_t i = 0; i < call->arg_count; ++i) {
                collectInstantiations(call->args[i]);
            }
            break;
        }
        case HIRExpr::Kind::BinOp: {
            auto* bin = static_cast<HIRBinOpExpr*>(expr);
            collectInstantiations(bin->lhs);
            collectInstantiations(bin->rhs);
            break;
        }
        case HIRExpr::Kind::UnaryOp: {
            auto* un = static_cast<HIRUnaryOpExpr*>(expr);
            collectInstantiations(un->operand);
            break;
        }
        case HIRExpr::Kind::If: {
            auto* iff = static_cast<HIRIfExpr*>(expr);
            collectInstantiations(iff->condition);
            collectInstantiations(iff->then_branch);
            collectInstantiations(iff->else_branch);
            break;
        }
        case HIRExpr::Kind::Block: {
            auto* blk = static_cast<HIRBlockExpr*>(expr);
            for (uint32_t i = 0; i < blk->stmt_count; ++i) {
                auto* stmt = blk->stmts[i];
                switch (stmt->kind) {
                    case HIRStmt::Kind::ValDecl:
                        collectInstantiations(static_cast<HIRValDeclStmt*>(stmt)->init);
                        break;
                    case HIRStmt::Kind::VarDecl:
                        collectInstantiations(static_cast<HIRVarDeclStmt*>(stmt)->init);
                        break;
                    case HIRStmt::Kind::ExprStmt:
                        collectInstantiations(static_cast<HIRExprStmt*>(stmt)->expr);
                        break;
                    case HIRStmt::Kind::Assign:
                        collectInstantiations(static_cast<HIRAssignStmt*>(stmt)->value);
                        break;
                    case HIRStmt::Kind::FieldAssign:
                        collectInstantiations(static_cast<HIRFieldAssignStmt*>(stmt)->target);
                        collectInstantiations(static_cast<HIRFieldAssignStmt*>(stmt)->value);
                        break;
                    case HIRStmt::Kind::DerefAssign:
                        collectInstantiations(static_cast<HIRDerefAssignStmt*>(stmt)->target);
                        collectInstantiations(static_cast<HIRDerefAssignStmt*>(stmt)->value);
                        break;
                    case HIRStmt::Kind::IndexAssign: {
                        auto* ia = static_cast<HIRIndexAssignStmt*>(stmt);
                        collectInstantiations(ia->array);
                        collectInstantiations(ia->index);
                        collectInstantiations(ia->value);
                        break;
                    }
                }
            }
            collectInstantiations(blk->result);
            break;
        }
        case HIRExpr::Kind::Match: {
            auto* m = static_cast<HIRMatchExpr*>(expr);
            collectInstantiations(m->scrutinee);
            for (uint32_t i = 0; i < m->arm_count; ++i) {
                collectInstantiations(m->arms[i].body);
            }
            break;
        }
        case HIRExpr::Kind::Return: {
            auto* ret = static_cast<HIRReturnExpr*>(expr);
            collectInstantiations(ret->value);
            break;
        }
        case HIRExpr::Kind::Loop: {
            auto* loop = static_cast<HIRLoopExpr*>(expr);
            for (uint32_t i = 0; i < loop->binding_count; ++i) {
                collectInstantiations(loop->bindings[i].init);
            }
            collectInstantiations(loop->body);
            break;
        }
        case HIRExpr::Kind::Break: {
            auto* brk = static_cast<HIRBreakExpr*>(expr);
            collectInstantiations(brk->value);
            break;
        }
        case HIRExpr::Kind::Continue: {
            auto* cont = static_cast<HIRContinueExpr*>(expr);
            for (uint32_t i = 0; i < cont->arg_count; ++i) {
                collectInstantiations(cont->args[i]);
            }
            break;
        }
        case HIRExpr::Kind::StructLit: {
            auto* sl = static_cast<HIRStructLitExpr*>(expr);
            for (uint32_t i = 0; i < sl->field_count; ++i) {
                collectInstantiations(sl->fields[i].value);
            }
            break;
        }
        case HIRExpr::Kind::FieldAccess: {
            auto* fa = static_cast<HIRFieldAccessExpr*>(expr);
            collectInstantiations(fa->object);
            break;
        }
        case HIRExpr::Kind::AddrOf: {
            auto* ao = static_cast<HIRAddrOfExpr*>(expr);
            collectInstantiations(ao->operand);
            break;
        }
        case HIRExpr::Kind::Deref: {
            auto* d = static_cast<HIRDerefExpr*>(expr);
            collectInstantiations(d->operand);
            break;
        }
        case HIRExpr::Kind::Cast: {
            auto* c = static_cast<HIRCastExpr*>(expr);
            collectInstantiations(c->operand);
            break;
        }
        case HIRExpr::Kind::ArrayLit: {
            auto* al = static_cast<HIRArrayLitExpr*>(expr);
            for (uint32_t i = 0; i < al->element_count; ++i) {
                collectInstantiations(al->elements[i]);
            }
            break;
        }
        case HIRExpr::Kind::IndexAccess: {
            auto* ia = static_cast<HIRIndexAccessExpr*>(expr);
            collectInstantiations(ia->array);
            collectInstantiations(ia->index);
            break;
        }
        case HIRExpr::Kind::CallIndirect: {
            auto* ci = static_cast<HIRCallIndirectExpr*>(expr);
            collectInstantiations(ci->callee);
            for (uint32_t i = 0; i < ci->arg_count; ++i) {
                collectInstantiations(ci->args[i]);
            }
            break;
        }
        case HIRExpr::Kind::UnionVariant: {
            auto* uv = static_cast<HIRUnionVariantExpr*>(expr);
            collectInstantiations(uv->payload);
            break;
        }
        // Leaf nodes — no recursion needed
        case HIRExpr::Kind::IntLit:
        case HIRExpr::Kind::FloatLit:
        case HIRExpr::Kind::BoolLit:
        case HIRExpr::Kind::StringLit:
        case HIRExpr::Kind::Ident:
        case HIRExpr::Kind::EnumAccess:
        case HIRExpr::Kind::InlineAsm:
        case HIRExpr::Kind::FnRef:
            break;
    }
}

// ============================================================================
// Type inference
// ============================================================================

// Recursively match a (possibly generic) expected type against a concrete actual type,
// extracting TypeVar → concrete substitutions.
static void deepInfer(const TypeTable& types, TypeId expected, TypeId actual,
                      std::unordered_map<TypeId, TypeId>& inferred) {
    if (expected == actual) return;

    auto& ei = types.get(expected);
    if (ei.kind == TypeKind::TypeVar) {
        if (!inferred.count(expected)) {
            inferred[expected] = actual;
        }
        return;
    }

    auto& ai = types.get(actual);
    if (ei.kind != ai.kind) return;

    switch (ei.kind) {
        case TypeKind::Union:
            // Match variant payload types
            for (uint32_t i = 0; i < ei.union_.variant_count && i < ai.union_.variant_count; ++i) {
                if (ei.union_.variants[i].payload_type != INVALID_TYPE &&
                    ai.union_.variants[i].payload_type != INVALID_TYPE) {
                    deepInfer(types, ei.union_.variants[i].payload_type,
                              ai.union_.variants[i].payload_type, inferred);
                }
            }
            break;
        case TypeKind::Struct:
            for (uint32_t i = 0; i < ei.struct_.field_count && i < ai.struct_.field_count; ++i) {
                deepInfer(types, ei.struct_.fields[i].type, ai.struct_.fields[i].type, inferred);
            }
            break;
        case TypeKind::Ptr:
        case TypeKind::PtrMut:
            deepInfer(types, ei.ptr.pointee, ai.ptr.pointee, inferred);
            break;
        case TypeKind::Array:
            deepInfer(types, ei.array.element, ai.array.element, inferred);
            break;
        case TypeKind::Fn:
            for (uint32_t i = 0; i < ei.fn.param_count && i < ai.fn.param_count; ++i) {
                deepInfer(types, ei.fn.params[i], ai.fn.params[i], inferred);
            }
            deepInfer(types, ei.fn.return_type, ai.fn.return_type, inferred);
            break;
        default:
            break;
    }
}

std::vector<TypeId> MonomorphizationPass::inferTypeArgs(
    HIRFnDecl* generic, const std::vector<TypeId>& arg_types) {

    // Build TypeVar → concrete type mapping by matching param types to arg types
    std::unordered_map<TypeId, TypeId> inferred;

    for (uint32_t i = 0; i < generic->param_count && i < arg_types.size(); ++i) {
        deepInfer(ctx_.types, generic->params[i].type, arg_types[i], inferred);
    }

    // Build result in order of type params
    std::vector<TypeId> result;
    for (uint32_t i = 0; i < generic->type_param_count; ++i) {
        TypeId tv_id = generic->type_params[i].type_var_id;
        auto it = inferred.find(tv_id);
        if (it != inferred.end()) {
            result.push_back(it->second);
        } else {
            // Could not infer — skip this instantiation
            return {};
        }
    }
    return result;
}

// ============================================================================
// Name mangling
// ============================================================================

std::string MonomorphizationPass::mangleName(
    std::string_view base, const std::vector<TypeId>& type_args) {
    std::string result(base);
    for (auto tid : type_args) {
        result += "_";
        result += ctx_.types.name(tid);
    }
    return result;
}

// ============================================================================
// Phase 2: Specialization (deep copy + type substitution)
// ============================================================================

TypeId MonomorphizationPass::substituteType(
    TypeId type, const std::unordered_map<TypeId, TypeId>& subst) {
    auto it = subst.find(type);
    if (it != subst.end()) return it->second;

    auto& ti = ctx_.types.get(type);
    switch (ti.kind) {
        case TypeKind::Ptr:
        case TypeKind::PtrMut: {
            TypeId inner = substituteType(ti.ptr.pointee, subst);
            if (inner == ti.ptr.pointee) return type;
            return ctx_.types.makePtr(inner, ti.kind == TypeKind::PtrMut);
        }
        case TypeKind::Fn: {
            bool changed = false;
            std::vector<TypeId> params;
            for (uint32_t i = 0; i < ti.fn.param_count; ++i) {
                TypeId p = substituteType(ti.fn.params[i], subst);
                params.push_back(p);
                if (p != ti.fn.params[i]) changed = true;
            }
            TypeId ret = substituteType(ti.fn.return_type, subst);
            if (ret != ti.fn.return_type) changed = true;
            if (!changed) return type;
            return ctx_.types.makeFn(params, ret, ti.fn.effects);
        }
        case TypeKind::Array: {
            TypeId elem = substituteType(ti.array.element, subst);
            if (elem == ti.array.element) return type;
            return ctx_.types.makeArrayType(elem, ti.array.count);
        }
        case TypeKind::Union: {
            bool changed = false;
            std::vector<VariantInfo> variants;
            for (uint32_t i = 0; i < ti.union_.variant_count; ++i) {
                VariantInfo vi = ti.union_.variants[i];
                if (vi.payload_type != INVALID_TYPE) {
                    TypeId sub = substituteType(vi.payload_type, subst);
                    if (sub != vi.payload_type) { vi.payload_type = sub; changed = true; }
                }
                variants.push_back(vi);
            }
            if (!changed) return type;
            // Build substituted name: e.g. Result_i64_i64
            std::string name(ti.union_.name);
            for (auto& [tv, concrete] : subst) {
                name += "_";
                name += ctx_.types.name(concrete);
            }
            return ctx_.types.makeUnion(ctx_.strings.intern(name), variants);
        }
        case TypeKind::Struct: {
            bool changed = false;
            std::vector<FieldInfo> fields;
            for (uint32_t i = 0; i < ti.struct_.field_count; ++i) {
                FieldInfo fi = ti.struct_.fields[i];
                TypeId sub = substituteType(fi.type, subst);
                if (sub != fi.type) { fi.type = sub; changed = true; }
                fields.push_back(fi);
            }
            if (!changed) return type;
            std::string name(ti.struct_.name);
            for (auto& [tv, concrete] : subst) {
                name += "_";
                name += ctx_.types.name(concrete);
            }
            return ctx_.types.makeStruct(ctx_.strings.intern(name), fields,
                                         ti.struct_.is_packed);
        }
        default:
            return type;
    }
}

HIRFnDecl* MonomorphizationPass::specialize(
    HIRFnDecl* generic, const std::vector<TypeId>& type_args,
    std::string_view mangled_name) {

    // Build substitution map: TypeVar TypeId → concrete TypeId
    std::unordered_map<TypeId, TypeId> subst;
    for (uint32_t i = 0; i < generic->type_param_count && i < type_args.size(); ++i) {
        subst[generic->type_params[i].type_var_id] = type_args[i];
    }

    auto* spec = ctx_.arena.make<HIRFnDecl>();
    spec->name = ctx_.strings.intern(mangled_name);
    spec->param_count = generic->param_count;
    spec->params = ctx_.arena.makeArray<HIRParam>(generic->param_count);
    for (uint32_t i = 0; i < generic->param_count; ++i) {
        spec->params[i].name = generic->params[i].name;
        spec->params[i].type = substituteType(generic->params[i].type, subst);
        spec->params[i].loc = generic->params[i].loc;
    }
    spec->return_type = substituteType(generic->return_type, subst);
    spec->type_param_count = 0;  // specialized — no longer generic
    spec->type_params = nullptr;
    spec->purity = generic->purity;
    spec->is_recursive = generic->is_recursive;
    spec->is_tail_recursive = generic->is_tail_recursive;
    spec->is_intrinsic = generic->is_intrinsic;
    spec->is_naked = generic->is_naked;
    spec->is_interrupt = generic->is_interrupt;
    spec->loc = generic->loc;

    // Deep copy body with type substitution
    spec->body = cloneExpr(generic->body, subst);

    return spec;
}

// ============================================================================
// Deep copy + substitution
// ============================================================================

HIRExpr* MonomorphizationPass::cloneExpr(
    HIRExpr* expr, const std::unordered_map<TypeId, TypeId>& subst) {
    if (!expr) return nullptr;

    switch (expr->kind) {
        case HIRExpr::Kind::IntLit: {
            auto* src = static_cast<HIRIntLitExpr*>(expr);
            auto* dst = ctx_.arena.make<HIRIntLitExpr>();
            dst->kind = src->kind;
            dst->type = substituteType(src->type, subst);
            dst->loc = src->loc;
            dst->value = src->value;
            return dst;
        }
        case HIRExpr::Kind::FloatLit: {
            auto* src = static_cast<HIRFloatLitExpr*>(expr);
            auto* dst = ctx_.arena.make<HIRFloatLitExpr>();
            dst->kind = src->kind;
            dst->type = substituteType(src->type, subst);
            dst->loc = src->loc;
            dst->value = src->value;
            return dst;
        }
        case HIRExpr::Kind::BoolLit: {
            auto* src = static_cast<HIRBoolLitExpr*>(expr);
            auto* dst = ctx_.arena.make<HIRBoolLitExpr>();
            dst->kind = src->kind;
            dst->type = substituteType(src->type, subst);
            dst->loc = src->loc;
            dst->value = src->value;
            return dst;
        }
        case HIRExpr::Kind::StringLit: {
            auto* src = static_cast<HIRStringLitExpr*>(expr);
            auto* dst = ctx_.arena.make<HIRStringLitExpr>();
            dst->kind = src->kind;
            dst->type = substituteType(src->type, subst);
            dst->loc = src->loc;
            dst->data = src->data;
            dst->length = src->length;
            return dst;
        }
        case HIRExpr::Kind::Ident: {
            auto* src = static_cast<HIRIdentExpr*>(expr);
            auto* dst = ctx_.arena.make<HIRIdentExpr>();
            dst->kind = src->kind;
            dst->type = substituteType(src->type, subst);
            dst->loc = src->loc;
            dst->name = src->name;
            return dst;
        }
        case HIRExpr::Kind::BinOp: {
            auto* src = static_cast<HIRBinOpExpr*>(expr);
            auto* dst = ctx_.arena.make<HIRBinOpExpr>();
            dst->kind = src->kind;
            dst->type = substituteType(src->type, subst);
            dst->loc = src->loc;
            dst->op = src->op;
            dst->lhs = cloneExpr(src->lhs, subst);
            dst->rhs = cloneExpr(src->rhs, subst);
            return dst;
        }
        case HIRExpr::Kind::UnaryOp: {
            auto* src = static_cast<HIRUnaryOpExpr*>(expr);
            auto* dst = ctx_.arena.make<HIRUnaryOpExpr>();
            dst->kind = src->kind;
            dst->type = substituteType(src->type, subst);
            dst->loc = src->loc;
            dst->op = src->op;
            dst->operand = cloneExpr(src->operand, subst);
            return dst;
        }
        case HIRExpr::Kind::Call: {
            auto* src = static_cast<HIRCallExpr*>(expr);
            auto* dst = ctx_.arena.make<HIRCallExpr>();
            dst->kind = src->kind;
            dst->type = substituteType(src->type, subst);
            dst->loc = src->loc;
            dst->callee = src->callee;
            dst->arg_count = src->arg_count;
            dst->is_tail_call = src->is_tail_call;
            dst->args = ctx_.arena.makeArray<HIRExpr*>(src->arg_count);
            for (uint32_t i = 0; i < src->arg_count; ++i) {
                dst->args[i] = cloneExpr(src->args[i], subst);
            }
            return dst;
        }
        case HIRExpr::Kind::If: {
            auto* src = static_cast<HIRIfExpr*>(expr);
            auto* dst = ctx_.arena.make<HIRIfExpr>();
            dst->kind = src->kind;
            dst->type = substituteType(src->type, subst);
            dst->loc = src->loc;
            dst->condition = cloneExpr(src->condition, subst);
            dst->then_branch = cloneExpr(src->then_branch, subst);
            dst->else_branch = cloneExpr(src->else_branch, subst);
            return dst;
        }
        case HIRExpr::Kind::Match: {
            auto* src = static_cast<HIRMatchExpr*>(expr);
            auto* dst = ctx_.arena.make<HIRMatchExpr>();
            dst->kind = src->kind;
            dst->type = substituteType(src->type, subst);
            dst->loc = src->loc;
            dst->scrutinee = cloneExpr(src->scrutinee, subst);
            dst->arm_count = src->arm_count;
            dst->arms = ctx_.arena.makeArray<HIRMatchArm>(src->arm_count);
            for (uint32_t i = 0; i < src->arm_count; ++i) {
                dst->arms[i].pattern = clonePattern(src->arms[i].pattern, subst);
                dst->arms[i].guard = cloneExpr(src->arms[i].guard, subst);
                dst->arms[i].body = cloneExpr(src->arms[i].body, subst);
                dst->arms[i].loc = src->arms[i].loc;
            }
            return dst;
        }
        case HIRExpr::Kind::Block: {
            auto* src = static_cast<HIRBlockExpr*>(expr);
            auto* dst = ctx_.arena.make<HIRBlockExpr>();
            dst->kind = src->kind;
            dst->type = substituteType(src->type, subst);
            dst->loc = src->loc;
            dst->stmt_count = src->stmt_count;
            dst->stmts = ctx_.arena.makeArray<HIRStmt*>(src->stmt_count);
            for (uint32_t i = 0; i < src->stmt_count; ++i) {
                dst->stmts[i] = cloneStmt(src->stmts[i], subst);
            }
            dst->result = cloneExpr(src->result, subst);
            return dst;
        }
        case HIRExpr::Kind::Return: {
            auto* src = static_cast<HIRReturnExpr*>(expr);
            auto* dst = ctx_.arena.make<HIRReturnExpr>();
            dst->kind = src->kind;
            dst->type = substituteType(src->type, subst);
            dst->loc = src->loc;
            dst->value = cloneExpr(src->value, subst);
            return dst;
        }
        case HIRExpr::Kind::StructLit: {
            auto* src = static_cast<HIRStructLitExpr*>(expr);
            auto* dst = ctx_.arena.make<HIRStructLitExpr>();
            dst->kind = src->kind;
            dst->type = substituteType(src->type, subst);
            dst->loc = src->loc;
            dst->struct_name = src->struct_name;
            dst->field_count = src->field_count;
            dst->fields = ctx_.arena.makeArray<HIRFieldInit>(src->field_count);
            for (uint32_t i = 0; i < src->field_count; ++i) {
                dst->fields[i].name = src->fields[i].name;
                dst->fields[i].value = cloneExpr(src->fields[i].value, subst);
                dst->fields[i].loc = src->fields[i].loc;
            }
            return dst;
        }
        case HIRExpr::Kind::FieldAccess: {
            auto* src = static_cast<HIRFieldAccessExpr*>(expr);
            auto* dst = ctx_.arena.make<HIRFieldAccessExpr>();
            dst->kind = src->kind;
            dst->type = substituteType(src->type, subst);
            dst->loc = src->loc;
            dst->object = cloneExpr(src->object, subst);
            dst->field_name = src->field_name;
            return dst;
        }
        case HIRExpr::Kind::EnumAccess: {
            auto* src = static_cast<HIREnumAccessExpr*>(expr);
            auto* dst = ctx_.arena.make<HIREnumAccessExpr>();
            *dst = *src;
            dst->type = substituteType(src->type, subst);
            return dst;
        }
        case HIRExpr::Kind::UnionVariant: {
            auto* src = static_cast<HIRUnionVariantExpr*>(expr);
            auto* dst = ctx_.arena.make<HIRUnionVariantExpr>();
            dst->kind = src->kind;
            dst->type = substituteType(src->type, subst);
            dst->loc = src->loc;
            dst->union_name = src->union_name;
            dst->variant_name = src->variant_name;
            dst->payload = cloneExpr(src->payload, subst);
            return dst;
        }
        case HIRExpr::Kind::AddrOf: {
            auto* src = static_cast<HIRAddrOfExpr*>(expr);
            auto* dst = ctx_.arena.make<HIRAddrOfExpr>();
            dst->kind = src->kind;
            dst->type = substituteType(src->type, subst);
            dst->loc = src->loc;
            dst->operand = cloneExpr(src->operand, subst);
            dst->is_mutable = src->is_mutable;
            return dst;
        }
        case HIRExpr::Kind::Deref: {
            auto* src = static_cast<HIRDerefExpr*>(expr);
            auto* dst = ctx_.arena.make<HIRDerefExpr>();
            dst->kind = src->kind;
            dst->type = substituteType(src->type, subst);
            dst->loc = src->loc;
            dst->operand = cloneExpr(src->operand, subst);
            return dst;
        }
        case HIRExpr::Kind::Cast: {
            auto* src = static_cast<HIRCastExpr*>(expr);
            auto* dst = ctx_.arena.make<HIRCastExpr>();
            dst->kind = src->kind;
            dst->type = substituteType(src->type, subst);
            dst->loc = src->loc;
            dst->operand = cloneExpr(src->operand, subst);
            dst->target_type = substituteType(src->target_type, subst);
            return dst;
        }
        case HIRExpr::Kind::Loop: {
            auto* src = static_cast<HIRLoopExpr*>(expr);
            auto* dst = ctx_.arena.make<HIRLoopExpr>();
            dst->kind = src->kind;
            dst->type = substituteType(src->type, subst);
            dst->loc = src->loc;
            dst->binding_count = src->binding_count;
            dst->bindings = ctx_.arena.makeArray<HIRLoopBinding>(src->binding_count);
            for (uint32_t i = 0; i < src->binding_count; ++i) {
                dst->bindings[i].name = src->bindings[i].name;
                dst->bindings[i].type = substituteType(src->bindings[i].type, subst);
                dst->bindings[i].init = cloneExpr(src->bindings[i].init, subst);
                dst->bindings[i].loc = src->bindings[i].loc;
            }
            dst->body = cloneExpr(src->body, subst);
            return dst;
        }
        case HIRExpr::Kind::Break: {
            auto* src = static_cast<HIRBreakExpr*>(expr);
            auto* dst = ctx_.arena.make<HIRBreakExpr>();
            dst->kind = src->kind;
            dst->type = substituteType(src->type, subst);
            dst->loc = src->loc;
            dst->value = cloneExpr(src->value, subst);
            return dst;
        }
        case HIRExpr::Kind::Continue: {
            auto* src = static_cast<HIRContinueExpr*>(expr);
            auto* dst = ctx_.arena.make<HIRContinueExpr>();
            dst->kind = src->kind;
            dst->type = substituteType(src->type, subst);
            dst->loc = src->loc;
            dst->arg_count = src->arg_count;
            dst->args = ctx_.arena.makeArray<HIRExpr*>(src->arg_count);
            for (uint32_t i = 0; i < src->arg_count; ++i) {
                dst->args[i] = cloneExpr(src->args[i], subst);
            }
            return dst;
        }
        case HIRExpr::Kind::ArrayLit: {
            auto* src = static_cast<HIRArrayLitExpr*>(expr);
            auto* dst = ctx_.arena.make<HIRArrayLitExpr>();
            dst->kind = src->kind;
            dst->type = substituteType(src->type, subst);
            dst->loc = src->loc;
            dst->element_count = src->element_count;
            dst->elements = ctx_.arena.makeArray<HIRExpr*>(src->element_count);
            for (uint32_t i = 0; i < src->element_count; ++i) {
                dst->elements[i] = cloneExpr(src->elements[i], subst);
            }
            return dst;
        }
        case HIRExpr::Kind::IndexAccess: {
            auto* src = static_cast<HIRIndexAccessExpr*>(expr);
            auto* dst = ctx_.arena.make<HIRIndexAccessExpr>();
            dst->kind = src->kind;
            dst->type = substituteType(src->type, subst);
            dst->loc = src->loc;
            dst->array = cloneExpr(src->array, subst);
            dst->index = cloneExpr(src->index, subst);
            return dst;
        }
        case HIRExpr::Kind::InlineAsm: {
            auto* src = static_cast<HIRInlineAsmExpr*>(expr);
            auto* dst = ctx_.arena.make<HIRInlineAsmExpr>();
            *dst = *src;
            return dst;
        }
        case HIRExpr::Kind::FnRef: {
            auto* src = static_cast<HIRFnRefExpr*>(expr);
            auto* dst = ctx_.arena.make<HIRFnRefExpr>();
            dst->kind = src->kind;
            dst->type = substituteType(src->type, subst);
            dst->loc = src->loc;
            dst->fn_name = src->fn_name;
            return dst;
        }
        case HIRExpr::Kind::CallIndirect: {
            auto* src = static_cast<HIRCallIndirectExpr*>(expr);
            auto* dst = ctx_.arena.make<HIRCallIndirectExpr>();
            dst->kind = src->kind;
            dst->type = substituteType(src->type, subst);
            dst->loc = src->loc;
            dst->callee = cloneExpr(src->callee, subst);
            dst->arg_count = src->arg_count;
            dst->is_tail_call = src->is_tail_call;
            dst->args = ctx_.arena.makeArray<HIRExpr*>(src->arg_count);
            for (uint32_t i = 0; i < src->arg_count; ++i) {
                dst->args[i] = cloneExpr(src->args[i], subst);
            }
            return dst;
        }
    }
    return expr;  // fallback
}

HIRStmt* MonomorphizationPass::cloneStmt(
    HIRStmt* stmt, const std::unordered_map<TypeId, TypeId>& subst) {
    if (!stmt) return nullptr;

    switch (stmt->kind) {
        case HIRStmt::Kind::ValDecl: {
            auto* src = static_cast<HIRValDeclStmt*>(stmt);
            auto* dst = ctx_.arena.make<HIRValDeclStmt>();
            dst->kind = src->kind;
            dst->loc = src->loc;
            dst->name = src->name;
            dst->type = substituteType(src->type, subst);
            dst->init = cloneExpr(src->init, subst);
            return dst;
        }
        case HIRStmt::Kind::VarDecl: {
            auto* src = static_cast<HIRVarDeclStmt*>(stmt);
            auto* dst = ctx_.arena.make<HIRVarDeclStmt>();
            dst->kind = src->kind;
            dst->loc = src->loc;
            dst->name = src->name;
            dst->type = substituteType(src->type, subst);
            dst->init = cloneExpr(src->init, subst);
            return dst;
        }
        case HIRStmt::Kind::ExprStmt: {
            auto* src = static_cast<HIRExprStmt*>(stmt);
            auto* dst = ctx_.arena.make<HIRExprStmt>();
            dst->kind = src->kind;
            dst->loc = src->loc;
            dst->expr = cloneExpr(src->expr, subst);
            return dst;
        }
        case HIRStmt::Kind::Assign: {
            auto* src = static_cast<HIRAssignStmt*>(stmt);
            auto* dst = ctx_.arena.make<HIRAssignStmt>();
            dst->kind = src->kind;
            dst->loc = src->loc;
            dst->name = src->name;
            dst->value = cloneExpr(src->value, subst);
            return dst;
        }
        case HIRStmt::Kind::FieldAssign: {
            auto* src = static_cast<HIRFieldAssignStmt*>(stmt);
            auto* dst = ctx_.arena.make<HIRFieldAssignStmt>();
            dst->kind = src->kind;
            dst->loc = src->loc;
            dst->target = cloneExpr(src->target, subst);
            dst->value = cloneExpr(src->value, subst);
            return dst;
        }
        case HIRStmt::Kind::DerefAssign: {
            auto* src = static_cast<HIRDerefAssignStmt*>(stmt);
            auto* dst = ctx_.arena.make<HIRDerefAssignStmt>();
            dst->kind = src->kind;
            dst->loc = src->loc;
            dst->target = cloneExpr(src->target, subst);
            dst->value = cloneExpr(src->value, subst);
            return dst;
        }
        case HIRStmt::Kind::IndexAssign: {
            auto* src = static_cast<HIRIndexAssignStmt*>(stmt);
            auto* dst = ctx_.arena.make<HIRIndexAssignStmt>();
            dst->kind = src->kind;
            dst->loc = src->loc;
            dst->array = cloneExpr(src->array, subst);
            dst->index = cloneExpr(src->index, subst);
            dst->value = cloneExpr(src->value, subst);
            return dst;
        }
    }
    return stmt;
}

HIRPattern* MonomorphizationPass::clonePattern(
    HIRPattern* pat, const std::unordered_map<TypeId, TypeId>& subst) {
    if (!pat) return nullptr;

    switch (pat->kind) {
        case HIRPattern::Kind::IntLit: {
            auto* src = static_cast<HIRIntLitPattern*>(pat);
            auto* dst = ctx_.arena.make<HIRIntLitPattern>();
            *dst = *src;
            dst->type = substituteType(src->type, subst);
            return dst;
        }
        case HIRPattern::Kind::BoolLit: {
            auto* src = static_cast<HIRBoolLitPattern*>(pat);
            auto* dst = ctx_.arena.make<HIRBoolLitPattern>();
            *dst = *src;
            dst->type = substituteType(src->type, subst);
            return dst;
        }
        case HIRPattern::Kind::Wildcard: {
            auto* dst = ctx_.arena.make<HIRWildcardPattern>();
            *dst = *static_cast<HIRWildcardPattern*>(pat);
            dst->type = substituteType(pat->type, subst);
            return dst;
        }
        case HIRPattern::Kind::Variable: {
            auto* src = static_cast<HIRVariablePattern*>(pat);
            auto* dst = ctx_.arena.make<HIRVariablePattern>();
            *dst = *src;
            dst->type = substituteType(src->type, subst);
            return dst;
        }
        case HIRPattern::Kind::Enum: {
            auto* src = static_cast<HIREnumPattern*>(pat);
            auto* dst = ctx_.arena.make<HIREnumPattern>();
            *dst = *src;
            dst->type = substituteType(src->type, subst);
            return dst;
        }
        case HIRPattern::Kind::Range: {
            auto* src = static_cast<HIRRangePattern*>(pat);
            auto* dst = ctx_.arena.make<HIRRangePattern>();
            *dst = *src;
            dst->type = substituteType(src->type, subst);
            return dst;
        }
        case HIRPattern::Kind::Union: {
            auto* src = static_cast<HIRUnionPattern*>(pat);
            auto* dst = ctx_.arena.make<HIRUnionPattern>();
            *dst = *src;
            dst->type = substituteType(src->type, subst);
            dst->inner = clonePattern(src->inner, subst);
            return dst;
        }
    }
    return pat;
}

// ============================================================================
// Phase 3: Patch call sites
// ============================================================================

void MonomorphizationPass::patchCallSites(HIRExpr* expr) {
    if (!expr) return;

    switch (expr->kind) {
        case HIRExpr::Kind::Call: {
            auto* call = static_cast<HIRCallExpr*>(expr);
            auto it = generic_fns_.find(call->callee);
            if (it != generic_fns_.end()) {
                // Infer type args
                std::vector<TypeId> arg_types;
                for (uint32_t i = 0; i < call->arg_count; ++i) {
                    arg_types.push_back(call->args[i]->type);
                }
                auto type_args = inferTypeArgs(it->second, arg_types);
                if (!type_args.empty()) {
                    auto mangled = mangleName(call->callee, type_args);
                    call->callee = ctx_.strings.intern(mangled);
                }
            }
            for (uint32_t i = 0; i < call->arg_count; ++i) {
                patchCallSites(call->args[i]);
            }
            break;
        }
        case HIRExpr::Kind::BinOp: {
            auto* bin = static_cast<HIRBinOpExpr*>(expr);
            patchCallSites(bin->lhs);
            patchCallSites(bin->rhs);
            break;
        }
        case HIRExpr::Kind::UnaryOp: {
            auto* un = static_cast<HIRUnaryOpExpr*>(expr);
            patchCallSites(un->operand);
            break;
        }
        case HIRExpr::Kind::If: {
            auto* iff = static_cast<HIRIfExpr*>(expr);
            patchCallSites(iff->condition);
            patchCallSites(iff->then_branch);
            patchCallSites(iff->else_branch);
            break;
        }
        case HIRExpr::Kind::Block: {
            auto* blk = static_cast<HIRBlockExpr*>(expr);
            for (uint32_t i = 0; i < blk->stmt_count; ++i) {
                patchCallSitesInStmt(blk->stmts[i]);
            }
            patchCallSites(blk->result);
            break;
        }
        case HIRExpr::Kind::Match: {
            auto* m = static_cast<HIRMatchExpr*>(expr);
            patchCallSites(m->scrutinee);
            for (uint32_t i = 0; i < m->arm_count; ++i) {
                patchCallSites(m->arms[i].body);
            }
            break;
        }
        case HIRExpr::Kind::Return: {
            patchCallSites(static_cast<HIRReturnExpr*>(expr)->value);
            break;
        }
        case HIRExpr::Kind::Loop: {
            auto* loop = static_cast<HIRLoopExpr*>(expr);
            for (uint32_t i = 0; i < loop->binding_count; ++i) {
                patchCallSites(loop->bindings[i].init);
            }
            patchCallSites(loop->body);
            break;
        }
        case HIRExpr::Kind::Break:
            patchCallSites(static_cast<HIRBreakExpr*>(expr)->value);
            break;
        case HIRExpr::Kind::Continue: {
            auto* cont = static_cast<HIRContinueExpr*>(expr);
            for (uint32_t i = 0; i < cont->arg_count; ++i) {
                patchCallSites(cont->args[i]);
            }
            break;
        }
        case HIRExpr::Kind::StructLit: {
            auto* sl = static_cast<HIRStructLitExpr*>(expr);
            for (uint32_t i = 0; i < sl->field_count; ++i) {
                patchCallSites(sl->fields[i].value);
            }
            break;
        }
        case HIRExpr::Kind::FieldAccess:
            patchCallSites(static_cast<HIRFieldAccessExpr*>(expr)->object);
            break;
        case HIRExpr::Kind::AddrOf:
            patchCallSites(static_cast<HIRAddrOfExpr*>(expr)->operand);
            break;
        case HIRExpr::Kind::Deref:
            patchCallSites(static_cast<HIRDerefExpr*>(expr)->operand);
            break;
        case HIRExpr::Kind::Cast:
            patchCallSites(static_cast<HIRCastExpr*>(expr)->operand);
            break;
        case HIRExpr::Kind::ArrayLit: {
            auto* al = static_cast<HIRArrayLitExpr*>(expr);
            for (uint32_t i = 0; i < al->element_count; ++i) {
                patchCallSites(al->elements[i]);
            }
            break;
        }
        case HIRExpr::Kind::IndexAccess: {
            auto* ia = static_cast<HIRIndexAccessExpr*>(expr);
            patchCallSites(ia->array);
            patchCallSites(ia->index);
            break;
        }
        case HIRExpr::Kind::CallIndirect: {
            auto* ci = static_cast<HIRCallIndirectExpr*>(expr);
            patchCallSites(ci->callee);
            for (uint32_t i = 0; i < ci->arg_count; ++i) {
                patchCallSites(ci->args[i]);
            }
            break;
        }
        case HIRExpr::Kind::UnionVariant:
            patchCallSites(static_cast<HIRUnionVariantExpr*>(expr)->payload);
            break;
        // Leaf nodes
        case HIRExpr::Kind::IntLit:
        case HIRExpr::Kind::FloatLit:
        case HIRExpr::Kind::BoolLit:
        case HIRExpr::Kind::StringLit:
        case HIRExpr::Kind::Ident:
        case HIRExpr::Kind::EnumAccess:
        case HIRExpr::Kind::InlineAsm:
        case HIRExpr::Kind::FnRef:
            break;
    }
}

void MonomorphizationPass::patchCallSitesInStmt(HIRStmt* stmt) {
    if (!stmt) return;
    switch (stmt->kind) {
        case HIRStmt::Kind::ValDecl:
            patchCallSites(static_cast<HIRValDeclStmt*>(stmt)->init);
            break;
        case HIRStmt::Kind::VarDecl:
            patchCallSites(static_cast<HIRVarDeclStmt*>(stmt)->init);
            break;
        case HIRStmt::Kind::ExprStmt:
            patchCallSites(static_cast<HIRExprStmt*>(stmt)->expr);
            break;
        case HIRStmt::Kind::Assign:
            patchCallSites(static_cast<HIRAssignStmt*>(stmt)->value);
            break;
        case HIRStmt::Kind::FieldAssign:
            patchCallSites(static_cast<HIRFieldAssignStmt*>(stmt)->target);
            patchCallSites(static_cast<HIRFieldAssignStmt*>(stmt)->value);
            break;
        case HIRStmt::Kind::DerefAssign:
            patchCallSites(static_cast<HIRDerefAssignStmt*>(stmt)->target);
            patchCallSites(static_cast<HIRDerefAssignStmt*>(stmt)->value);
            break;
        case HIRStmt::Kind::IndexAssign: {
            auto* ia = static_cast<HIRIndexAssignStmt*>(stmt);
            patchCallSites(ia->array);
            patchCallSites(ia->index);
            patchCallSites(ia->value);
            break;
        }
    }
}

} // namespace kern
