#include "kern/hir/HIRBuilder.h"
#include <algorithm>
#include <cstring>
#include <string>

namespace kern {

// ============================================================================
// Type query helpers (using TypeTable)
// ============================================================================

bool HIRBuilder::isIntegerType(TypeId id) const {
    return ctx_.types.isInteger(id);
}

bool HIRBuilder::isFloatType(TypeId id) const {
    return ctx_.types.isFloat(id);
}

bool HIRBuilder::isSignedType(TypeId id) const {
    return ctx_.types.isSigned(id);
}

static bool isPtrType(TypeId id, const TypeTable& types) {
    if (id >= types.size()) return false;
    auto k = types.get(id).kind;
    return k == TypeKind::Ptr || k == TypeKind::PtrMut;
}

// Get the actual type name (struct/enum/union use their declared name, not "struct"/"enum"/"union")
static std::string_view actualTypeName(TypeId id, const TypeTable& types) {
    if (id >= types.size()) return types.name(id);
    const auto& ti = types.get(id);
    if (ti.kind == TypeKind::Struct) return ti.struct_.name;
    if (ti.kind == TypeKind::Union) return ti.union_.name;
    if (ti.kind == TypeKind::Enum) return ti.enum_.name;
    return types.name(id);
}

// Never (!) is the bottom type — compatible with any other type.
static bool typesMatch(TypeId a, TypeId b) {
    if (a == b) return true;
    if (a == TypeTable::Never || b == TypeTable::Never) return true;
    return false;
}

// Extract the Fn type from a closure struct (first field = __fn)
static TypeId closureFnType(TypeId tid, const TypeTable& types,
                             const std::unordered_set<TypeId>& closure_types) {
    if (!closure_types.count(tid) || tid >= types.size()) return INVALID_TYPE;
    auto& si = types.get(tid);
    if (si.struct_.field_count > 0) return si.struct_.fields[0].type;
    return INVALID_TYPE;
}

// Check if types match, allowing closure struct → Fn coercion
static bool typesMatchClosure(TypeId a, TypeId b, const TypeTable& types,
                               const std::unordered_set<TypeId>& closure_types) {
    if (typesMatch(a, b)) return true;
    TypeId fn_a = closureFnType(a, types, closure_types);
    TypeId fn_b = closureFnType(b, types, closure_types);
    // Closure struct ↔ Fn: closure's __fn field matches the Fn type
    if (fn_a != INVALID_TYPE && b < types.size() && types.get(b).kind == TypeKind::Fn) {
        if (fn_a == b) return true;
    }
    if (fn_b != INVALID_TYPE && a < types.size() && types.get(a).kind == TypeKind::Fn) {
        if (fn_b == a) return true;
    }
    // Closure struct ↔ Closure struct: both have same __fn type
    if (fn_a != INVALID_TYPE && fn_b != INVALID_TYPE && fn_a == fn_b) return true;
    return false;
}

// Can type 'from' be implicitly widened to type 'to'?
// Only same-signedness integer widening is allowed (i8→i16→i32→i64, u8→u16→u32→u64).
static bool canImplicitlyWiden(TypeId from, TypeId to) {
    // Signed widening
    if (from == TypeTable::I8  && (to == TypeTable::I16 || to == TypeTable::I32 || to == TypeTable::I64)) return true;
    if (from == TypeTable::I16 && (to == TypeTable::I32 || to == TypeTable::I64)) return true;
    if (from == TypeTable::I32 && to == TypeTable::I64) return true;
    // Unsigned widening
    if (from == TypeTable::U8  && (to == TypeTable::U16 || to == TypeTable::U32 || to == TypeTable::U64)) return true;
    if (from == TypeTable::U16 && (to == TypeTable::U32 || to == TypeTable::U64)) return true;
    if (from == TypeTable::U32 && to == TypeTable::U64) return true;
    return false;
}

// Check if a type contains any TypeVar (for generic type matching)
static bool containsTypeVarImpl(const TypeTable& types, TypeId tid,
                                std::unordered_set<TypeId>& visited) {
    if (!visited.insert(tid).second) return false; // cycle — not a TypeVar
    const auto& info = types.get(tid);
    if (info.kind == TypeKind::TypeVar) return true;
    if (info.kind == TypeKind::Union) {
        for (uint32_t i = 0; i < info.union_.variant_count; ++i) {
            if (info.union_.variants[i].payload_type != INVALID_TYPE &&
                containsTypeVarImpl(types, info.union_.variants[i].payload_type, visited))
                return true;
        }
    }
    if (info.kind == TypeKind::Struct) {
        for (uint32_t i = 0; i < info.struct_.field_count; ++i) {
            if (containsTypeVarImpl(types, info.struct_.fields[i].type, visited))
                return true;
        }
    }
    if (info.kind == TypeKind::Ptr || info.kind == TypeKind::PtrMut) {
        return containsTypeVarImpl(types, info.ptr.pointee, visited);
    }
    if (info.kind == TypeKind::Array) {
        return containsTypeVarImpl(types, info.array.element, visited);
    }
    if (info.kind == TypeKind::Fn) {
        for (uint32_t i = 0; i < info.fn.param_count; ++i) {
            if (containsTypeVarImpl(types, info.fn.params[i], visited)) return true;
        }
        return containsTypeVarImpl(types, info.fn.return_type, visited);
    }
    return false;
}

static bool containsTypeVar(const TypeTable& types, TypeId tid) {
    std::unordered_set<TypeId> visited;
    return containsTypeVarImpl(types, tid, visited);
}

// Deep match: try to unify expected (may contain TypeVars) with actual (concrete)
// and extract TypeVar→concrete substitutions
static bool deepTypeMatch(const TypeTable& types, TypeId expected, TypeId actual,
                          std::unordered_map<TypeId, TypeId>& subst) {
    if (expected == actual) return true;
    const auto& ei = types.get(expected);
    if (ei.kind == TypeKind::TypeVar) {
        auto it = subst.find(expected);
        if (it != subst.end()) return it->second == actual;
        subst[expected] = actual;
        return true;
    }
    const auto& ai = types.get(actual);
    if (ei.kind != ai.kind) return false;
    if (ei.kind == TypeKind::Union) {
        if (ei.union_.variant_count != ai.union_.variant_count) return false;
        for (uint32_t i = 0; i < ei.union_.variant_count; ++i) {
            auto ep = ei.union_.variants[i].payload_type;
            auto ap = ai.union_.variants[i].payload_type;
            if (ep == INVALID_TYPE && ap == INVALID_TYPE) continue;
            if (ep == INVALID_TYPE || ap == INVALID_TYPE) return false;
            if (!deepTypeMatch(types, ep, ap, subst)) return false;
        }
        return true;
    }
    if (ei.kind == TypeKind::Struct) {
        if (ei.struct_.field_count != ai.struct_.field_count) return false;
        for (uint32_t i = 0; i < ei.struct_.field_count; ++i) {
            if (!deepTypeMatch(types, ei.struct_.fields[i].type, ai.struct_.fields[i].type, subst))
                return false;
        }
        return true;
    }
    if (ei.kind == TypeKind::Ptr || ei.kind == TypeKind::PtrMut) {
        return deepTypeMatch(types, ei.ptr.pointee, ai.ptr.pointee, subst);
    }
    if (ei.kind == TypeKind::Array) {
        return ei.array.count == ai.array.count &&
               deepTypeMatch(types, ei.array.element, ai.array.element, subst);
    }
    if (ei.kind == TypeKind::Fn) {
        if (ei.fn.param_count != ai.fn.param_count) return false;
        for (uint32_t i = 0; i < ei.fn.param_count; ++i) {
            if (!deepTypeMatch(types, ei.fn.params[i], ai.fn.params[i], subst))
                return false;
        }
        return deepTypeMatch(types, ei.fn.return_type, ai.fn.return_type, subst);
    }
    return false;
}

// Recursively substitute TypeVars in a type using the substitution map.
// Returns the substituted TypeId, creating new types as needed.
static TypeId substituteTypeVars(TypeTable& types, TypeId tid,
                                  const std::unordered_map<TypeId, TypeId>& subst) {
    auto it = subst.find(tid);
    if (it != subst.end()) return it->second;

    const auto& info = types.get(tid);
    if (info.kind == TypeKind::Fn) {
        bool changed = false;
        std::vector<TypeId> new_params(info.fn.param_count);
        for (uint32_t i = 0; i < info.fn.param_count; ++i) {
            new_params[i] = substituteTypeVars(types, info.fn.params[i], subst);
            if (new_params[i] != info.fn.params[i]) changed = true;
        }
        TypeId new_ret = substituteTypeVars(types, info.fn.return_type, subst);
        if (new_ret != info.fn.return_type) changed = true;
        if (!changed) return tid;
        return types.makeFn(new_params, new_ret);
    }
    // For other composite types (Struct, Union, Ptr, Array) we could add
    // substitution here, but currently generic structs/unions are handled
    // via separate monomorphization paths.
    return tid;
}

// When merging two branch types, pick the non-Never type.
static TypeId mergeTypes(TypeId a, TypeId b) {
    if (a == TypeTable::Never) return b;
    if (b == TypeTable::Never) return a;
    return a; // assumes they match
}

// Constant-fold an HIR expression to an integer value.
// Supports const fn calls by evaluating the body with argument substitution.
// Optional `env` provides variable name→value bindings (for const fn params).
bool HIRBuilder::constEvalInt(HIRExpr* expr, int64_t* out,
                              const std::unordered_map<std::string_view, int64_t>* env) {
    if (expr->kind == HIRExpr::Kind::IntLit) {
        *out = static_cast<HIRIntLitExpr*>(expr)->value;
        return true;
    }
    if (expr->kind == HIRExpr::Kind::BoolLit) {
        *out = static_cast<HIRBoolLitExpr*>(expr)->value ? 1 : 0;
        return true;
    }
    if (expr->kind == HIRExpr::Kind::Ident) {
        auto* id = static_cast<HIRIdentExpr*>(expr);
        if (env) {
            auto it = env->find(id->name);
            if (it != env->end()) { *out = it->second; return true; }
        }
        return false;
    }
    if (expr->kind == HIRExpr::Kind::BinOp) {
        auto* bin = static_cast<HIRBinOpExpr*>(expr);
        int64_t lv, rv;
        if (!constEvalInt(bin->lhs, &lv, env) || !constEvalInt(bin->rhs, &rv, env))
            return false;
        switch (bin->op) {
            case HIRBinOp::Add:   *out = lv + rv; return true;
            case HIRBinOp::Sub:   *out = lv - rv; return true;
            case HIRBinOp::Mul:   *out = lv * rv; return true;
            case HIRBinOp::Div:   if (rv == 0) return false; *out = lv / rv; return true;
            case HIRBinOp::Mod:   if (rv == 0) return false; *out = lv % rv; return true;
            case HIRBinOp::Eq:    *out = (lv == rv) ? 1 : 0; return true;
            case HIRBinOp::NotEq: *out = (lv != rv) ? 1 : 0; return true;
            case HIRBinOp::Lt:    *out = (lv < rv)  ? 1 : 0; return true;
            case HIRBinOp::LtEq:  *out = (lv <= rv) ? 1 : 0; return true;
            case HIRBinOp::Gt:    *out = (lv > rv)  ? 1 : 0; return true;
            case HIRBinOp::GtEq:  *out = (lv >= rv) ? 1 : 0; return true;
            case HIRBinOp::BitAnd: *out = lv & rv; return true;
            case HIRBinOp::BitOr:  *out = lv | rv; return true;
            case HIRBinOp::BitXor: *out = lv ^ rv; return true;
            case HIRBinOp::Shl:   *out = lv << rv; return true;
            case HIRBinOp::Shr:   *out = lv >> rv; return true;
            case HIRBinOp::And:   *out = (lv && rv) ? 1 : 0; return true;
            case HIRBinOp::Or:    *out = (lv || rv) ? 1 : 0; return true;
            case HIRBinOp::AddWrap: *out = lv + rv; return true;
            case HIRBinOp::SubWrap: *out = lv - rv; return true;
            case HIRBinOp::MulWrap: *out = lv * rv; return true;
            case HIRBinOp::AddSat:  *out = lv + rv; return true;
            case HIRBinOp::SubSat:  *out = lv - rv; return true;
        }
        return false;
    }
    if (expr->kind == HIRExpr::Kind::UnaryOp) {
        auto* un = static_cast<HIRUnaryOpExpr*>(expr);
        int64_t v;
        if (!constEvalInt(un->operand, &v, env)) return false;
        switch (un->op) {
            case HIRUnaryOp::Neg:    *out = -v; return true;
            case HIRUnaryOp::Not:    *out = v ? 0 : 1; return true;
            case HIRUnaryOp::BitNot: *out = ~v; return true;
            default: return false;
        }
    }
    if (expr->kind == HIRExpr::Kind::If) {
        auto* ife = static_cast<HIRIfExpr*>(expr);
        int64_t cond;
        if (!constEvalInt(ife->condition, &cond, env)) return false;
        if (cond != 0) return constEvalInt(ife->then_branch, out, env);
        if (ife->else_branch) return constEvalInt(ife->else_branch, out, env);
        return false;
    }
    if (expr->kind == HIRExpr::Kind::Block) {
        auto* blk = static_cast<HIRBlockExpr*>(expr);
        // Handle blocks with val/var bindings by extending the environment
        std::unordered_map<std::string_view, int64_t> local_env;
        if (env) local_env = *env;
        for (uint32_t i = 0; i < blk->stmt_count; ++i) {
            auto* stmt = blk->stmts[i];
            if (stmt->kind == HIRStmt::Kind::ValDecl) {
                auto* vs = static_cast<HIRValDeclStmt*>(stmt);
                int64_t val;
                if (!constEvalInt(vs->init, &val, &local_env)) return false;
                local_env[vs->name] = val;
            } else if (stmt->kind == HIRStmt::Kind::VarDecl) {
                auto* vs = static_cast<HIRVarDeclStmt*>(stmt);
                if (vs->init) {
                    int64_t val;
                    if (!constEvalInt(vs->init, &val, &local_env)) return false;
                    local_env[vs->name] = val;
                } else {
                    local_env[vs->name] = 0;
                }
            } else if (stmt->kind == HIRStmt::Kind::Assign) {
                auto* as = static_cast<HIRAssignStmt*>(stmt);
                int64_t val;
                if (!constEvalInt(as->value, &val, &local_env)) return false;
                local_env[as->name] = val;
            } else if (stmt->kind == HIRStmt::Kind::ExprStmt) {
                // Allow expression statements — skip for const eval
            } else {
                return false; // unsupported stmt kind
            }
        }
        if (blk->result)
            return constEvalInt(blk->result, out, &local_env);
        return false;
    }
    if (expr->kind == HIRExpr::Kind::Cast) {
        auto* cast = static_cast<HIRCastExpr*>(expr);
        return constEvalInt(cast->operand, out, env);
    }
    if (expr->kind == HIRExpr::Kind::Call) {
        auto* call = static_cast<HIRCallExpr*>(expr);
        auto it = hir_fns_.find(call->callee);
        if (it == hir_fns_.end()) return false;
        HIRFnDecl* fn = it->second;
        if (!fn->is_const || !fn->body) return false;
        // Evaluate all arguments as compile-time constants
        std::unordered_map<std::string_view, int64_t> call_env;
        for (uint32_t i = 0; i < call->arg_count && i < fn->param_count; ++i) {
            int64_t arg_val;
            if (!constEvalInt(call->args[i], &arg_val, env)) return false;
            call_env[fn->params[i].name] = arg_val;
        }
        return constEvalInt(fn->body, out, &call_env);
    }
    return false;
}

bool HIRBuilder::intFitsInType(int64_t value, TypeId type) const {
    if (!isIntegerType(type)) return false;
    auto bw = ctx_.types.bitWidth(type);
    bool is_signed = isSignedType(type);
    switch (bw) {
        case 8:  return is_signed ? (value >= -128 && value <= 127) : (value >= 0 && value <= 255);
        case 16: return is_signed ? (value >= -32768 && value <= 32767) : (value >= 0 && value <= 65535);
        case 32: return is_signed ? (value >= -2147483648LL && value <= 2147483647LL) : (value >= 0 && value <= 4294967295LL);
        case 64: return is_signed ? true : (value >= 0);
        default: return false;
    }
}

// ============================================================================
// Constructor
// ============================================================================

HIRBuilder::HIRBuilder(CompilationContext& ctx) : ctx_(ctx) {}

bool HIRBuilder::hasErrors() const {
    return ctx_.diag.hasErrors();
}

HIRExpr* HIRBuilder::errorExpr(SourceLocation loc) {
    auto* e = ctx_.arena.make<HIRExpr>();
    e->kind = HIRExpr::Kind::IntLit; // placeholder
    e->type = TypeTable::Error;
    e->loc = loc;
    return e;
}

HIRExpr* HIRBuilder::implicitWiden(HIRExpr* expr, TypeId target) {
    if (expr->type == target || expr->type == TypeTable::Error || target == TypeTable::Error) return expr;
    if (!canImplicitlyWiden(expr->type, target)) return expr;
    auto* cast = ctx_.arena.make<HIRCastExpr>();
    cast->kind = HIRExpr::Kind::Cast;
    cast->loc = expr->loc;
    cast->operand = expr;
    cast->target_type = target;
    cast->type = target;
    return cast;
}

// ============================================================================
// Type resolution
// ============================================================================

TypeId HIRBuilder::resolveType(const TypeRef& ref) {
    if (ref.kind == TypeRef::Kind::Ptr) {
        TypeId pointee = ref.pointee ? resolveType(*ref.pointee) : TypeTable::Error;
        return ctx_.types.makePtr(pointee, ref.is_ptr_var);
    }
    if (ref.kind == TypeRef::Kind::Never) {
        return TypeTable::Never;
    }
    if (ref.kind == TypeRef::Kind::Fn) {
        std::vector<TypeId> params;
        for (uint32_t i = 0; i < ref.fn_param_count; ++i)
            params.push_back(resolveType(ref.fn_params[i]));
        TypeId ret = ref.fn_return ? resolveType(*ref.fn_return) : TypeTable::Unit;
        return ctx_.types.makeFn(params, ret);
    }
    if (ref.kind == TypeRef::Kind::Array) {
        TypeId elem = ref.array_element ? resolveType(*ref.array_element) : TypeTable::Error;
        uint32_t size = ref.array_size;
        if (!ref.array_size_name.empty()) {
            auto it = const_values_.find(ref.array_size_name);
            if (it != const_values_.end()) {
                size = static_cast<uint32_t>(it->second);
            } else {
                ctx_.diag.error(ref.loc, std::string("unknown const generic parameter '") +
                    std::string(ref.array_size_name) + "'");
                return TypeTable::Error;
            }
        }
        return ctx_.types.makeArrayType(elem, size);
    }
    if (ref.kind == TypeRef::Kind::ConstVal) {
        // Const values in type position are not real types — handled during instantiation
        ctx_.diag.error(ref.loc, "integer constant cannot be used as a type");
        return TypeTable::Error;
    }
    if (ref.kind == TypeRef::Kind::Dyn) {
        // dyn TraitName — look up trait and create DynTrait type
        auto trait_it = trait_table_.find(ref.name);
        if (trait_it == trait_table_.end()) {
            ctx_.diag.error(ref.loc, std::string("unknown trait '") +
                std::string(ref.name) + "'");
            return TypeTable::Error;
        }
        return ctx_.types.makeDynTrait(ref.name, trait_it->second.method_names);
    }
    if (ref.name == "i8")   return TypeTable::I8;
    if (ref.name == "i16")  return TypeTable::I16;
    if (ref.name == "i32")  return TypeTable::I32;
    if (ref.name == "i64")  return TypeTable::I64;
    if (ref.name == "u8")   return TypeTable::U8;
    if (ref.name == "u16")  return TypeTable::U16;
    if (ref.name == "u32")  return TypeTable::U32;
    if (ref.name == "u64")  return TypeTable::U64;
    if (ref.name == "f32")  return TypeTable::F32;
    if (ref.name == "f64")  return TypeTable::F64;
    if (ref.name == "bool") return TypeTable::Bool;
    if (ref.name == "Unit") return TypeTable::Unit;
    if (ref.name == "String") {
        // String is a builtin struct-like type. Look up or register.
        auto it = named_types_.find("String");
        if (it != named_types_.end()) return it->second;
        // Register String as a struct: {data: Ptr<u8>, len: u64}
        FieldInfo fields[] = {
            {ctx_.strings.intern("data"), ctx_.types.makePtr(TypeTable::U8, false), false, 0},
            {ctx_.strings.intern("len"), TypeTable::U64, false, 8},
        };
        TypeId tid = ctx_.types.makeStruct(ctx_.strings.intern("String"), fields);
        named_types_["String"] = tid;
        return tid;
    }
    // Slice<T> = { data: Ptr<T>, len: u64 }
    if (ref.name == "Slice" && ref.pointee) {
        TypeId elem_type = resolveType(*ref.pointee);
        // Create a unique name for this slice instantiation
        std::string slice_name = "Slice_" + std::string(ctx_.types.name(elem_type));
        auto interned = ctx_.strings.intern(slice_name);
        auto it = named_types_.find(interned);
        if (it != named_types_.end()) return it->second;
        FieldInfo fields[] = {
            {ctx_.strings.intern("data"), ctx_.types.makePtr(elem_type, false), false, 0},
            {ctx_.strings.intern("len"), TypeTable::U64, false, 8},
        };
        TypeId tid = ctx_.types.makeStruct(interned, fields);
        named_types_[interned] = tid;
        return tid;
    }
    // Generic type instantiation: Name<T1, T2, ...> or Name<T1, 4, ...>
    if (ref.type_arg_count > 0) {
        // Resolve type arguments and extract const values
        std::vector<TypeId> type_args;
        std::vector<std::pair<uint32_t, int64_t>> const_arg_values; // index → value
        for (uint32_t i = 0; i < ref.type_arg_count; ++i) {
            if (ref.type_args[i].kind == TypeRef::Kind::ConstVal) {
                type_args.push_back(INVALID_TYPE); // placeholder
                const_arg_values.push_back({i, ref.type_args[i].const_value});
            } else {
                type_args.push_back(resolveType(ref.type_args[i]));
            }
        }

        // Build mangled name: Name_T1_4
        std::string mangled = std::string(ref.name);
        for (uint32_t i = 0; i < ref.type_arg_count; ++i) {
            mangled += "_";
            if (ref.type_args[i].kind == TypeRef::Kind::ConstVal) {
                mangled += std::to_string(ref.type_args[i].const_value);
            } else {
                mangled += ctx_.types.name(type_args[i]);
            }
        }
        auto interned_mangled = ctx_.strings.intern(mangled);

        // Check if already instantiated
        auto it = named_types_.find(interned_mangled);
        if (it != named_types_.end()) return it->second;

        // Look up generic struct template
        auto gs = generic_structs_.find(ref.name);
        if (gs != generic_structs_.end()) {
            auto* sd = gs->second;
            if (ref.type_arg_count != sd->type_param_count) {
                ctx_.diag.error(ref.loc, std::string("generic struct '") +
                    std::string(ref.name) + "' expects " +
                    std::to_string(sd->type_param_count) + " type arguments, got " +
                    std::to_string(ref.type_arg_count));
                return TypeTable::Error;
            }

            // Temporarily register type param and const param substitutions
            std::vector<std::pair<std::string_view, TypeId>> saved;
            std::vector<std::pair<std::string_view, int64_t>> saved_const;
            for (uint32_t i = 0; i < sd->type_param_count; ++i) {
                auto param_name = sd->type_params[i].name;
                if (sd->type_params[i].is_const) {
                    // Find the const value for this index
                    int64_t cv = 0;
                    for (auto& [idx, val] : const_arg_values) {
                        if (idx == i) { cv = val; break; }
                    }
                    if (const_values_.count(param_name)) {
                        saved_const.push_back({param_name, const_values_[param_name]});
                    }
                    const_values_[param_name] = cv;
                } else {
                    if (named_types_.count(param_name)) {
                        saved.push_back({param_name, named_types_[param_name]});
                    }
                    named_types_[param_name] = type_args[i];
                }
            }

            // Build concrete struct fields
            std::vector<FieldInfo> fields;
            for (uint32_t j = 0; j < sd->field_count; ++j) {
                auto& f = sd->fields[j];
                TypeId ft = resolveType(f.type);
                fields.push_back({ctx_.strings.intern(f.name), ft, f.is_mutable, -1,
                              f.bit_width, 0});
            }

            TypeId tid = ctx_.types.makeStruct(interned_mangled, fields,
                                                sd->is_packed, sd->explicit_align,
                                                sd->is_repr_c);
            named_types_[interned_mangled] = tid;

            // Restore type param and const param names
            for (uint32_t i = 0; i < sd->type_param_count; ++i) {
                if (sd->type_params[i].is_const) {
                    const_values_.erase(sd->type_params[i].name);
                } else {
                    named_types_.erase(sd->type_params[i].name);
                }
            }
            for (auto& [name, old_tid] : saved) {
                named_types_[name] = old_tid;
            }
            for (auto& [name, old_val] : saved_const) {
                const_values_[name] = old_val;
            }

            return tid;
        }

        // Look up generic union template
        auto gu = generic_unions_.find(ref.name);
        if (gu != generic_unions_.end()) {
            auto* ud = gu->second;
            if (ref.type_arg_count != ud->type_param_count) {
                ctx_.diag.error(ref.loc, std::string("generic union '") +
                    std::string(ref.name) + "' expects " +
                    std::to_string(ud->type_param_count) + " type arguments, got " +
                    std::to_string(ref.type_arg_count));
                return TypeTable::Error;
            }

            // Temporarily register type param and const param substitutions
            std::vector<std::pair<std::string_view, TypeId>> saved;
            std::vector<std::pair<std::string_view, int64_t>> saved_const;
            for (uint32_t i = 0; i < ud->type_param_count; ++i) {
                auto param_name = ud->type_params[i].name;
                if (ud->type_params[i].is_const) {
                    int64_t cv = 0;
                    for (auto& [idx, val] : const_arg_values) {
                        if (idx == i) { cv = val; break; }
                    }
                    if (const_values_.count(param_name)) {
                        saved_const.push_back({param_name, const_values_[param_name]});
                    }
                    const_values_[param_name] = cv;
                } else {
                    if (named_types_.count(param_name)) {
                        saved.push_back({param_name, named_types_[param_name]});
                    }
                    named_types_[param_name] = type_args[i];
                }
            }

            // Build concrete union variants
            std::vector<VariantInfo> variants;
            for (uint32_t j = 0; j < ud->variant_count; ++j) {
                TypeId payload = INVALID_TYPE;
                if (ud->variants[j].payload_type) {
                    payload = resolveType(*ud->variants[j].payload_type);
                }
                variants.push_back({ctx_.strings.intern(ud->variants[j].name), payload});
            }

            TypeId tid = ctx_.types.makeUnion(interned_mangled, variants, ud->is_repr_c);
            named_types_[interned_mangled] = tid;

            // Restore type param and const param names
            for (uint32_t i = 0; i < ud->type_param_count; ++i) {
                if (ud->type_params[i].is_const) {
                    const_values_.erase(ud->type_params[i].name);
                } else {
                    named_types_.erase(ud->type_params[i].name);
                }
            }
            for (auto& [name, old_tid] : saved) {
                named_types_[name] = old_tid;
            }
            for (auto& [name, old_val] : saved_const) {
                const_values_[name] = old_val;
            }

            return tid;
        }

        ctx_.diag.error(ref.loc, std::string("unknown generic type '") +
            std::string(ref.name) + "'");
        return TypeTable::Error;
    }

    auto it = named_types_.find(ref.name);
    if (it != named_types_.end()) return it->second;
    ctx_.diag.error(ref.loc, std::string("unknown type '") + std::string(ref.name) + "'");
    return TypeTable::Error;
}

TypeId HIRBuilder::resolvePointee(const TypeRef& ref) {
    if (ref.kind == TypeRef::Kind::Ptr && ref.pointee) {
        return resolveType(*ref.pointee);
    }
    return TypeTable::Error;
}

// ============================================================================
// Declaration registration
// ============================================================================

void HIRBuilder::registerStructDecls(const Module* ast) {
    // First pass: register opaque struct TypeIds so self-referential types resolve
    for (uint32_t i = 0; i < ast->struct_count; ++i) {
        auto* sd = ast->structs[i];
        if (sd->type_param_count > 0) {
            generic_structs_[sd->name] = sd;
            continue;
        }
        TypeId tid = ctx_.types.makeOpaqueStruct(ctx_.strings.intern(sd->name));
        named_types_[sd->name] = tid;
    }

    // Second pass: resolve fields and update struct layouts
    for (uint32_t i = 0; i < ast->struct_count; ++i) {
        auto* sd = ast->structs[i];
        if (sd->type_param_count > 0) continue;
        std::vector<FieldInfo> fields;

        for (uint32_t j = 0; j < sd->field_count; ++j) {
            auto& f = sd->fields[j];
            TypeId ft = resolveType(f.type);
            fields.push_back({ctx_.strings.intern(f.name), ft, f.is_mutable, -1,
                              f.bit_width, 0});
        }

        TypeId tid = named_types_[sd->name];
        ctx_.types.updateStruct(tid, fields, sd->is_packed, sd->explicit_align, sd->is_repr_c);
    }
}

void HIRBuilder::registerEnumDecls(const Module* ast) {
    for (uint32_t i = 0; i < ast->enum_count; ++i) {
        auto* ed = ast->enums[i];
        std::vector<std::string_view> names;
        std::vector<int64_t> values;
        int64_t next_value = 0;
        for (uint32_t j = 0; j < ed->variant_count; ++j) {
            names.push_back(ctx_.strings.intern(ed->variants[j].name));
            if (ed->variants[j].has_value) {
                next_value = ed->variants[j].value;
            }
            values.push_back(next_value);
            next_value++;
        }
        TypeId tid = ctx_.types.makeEnum(ctx_.strings.intern(ed->name), names, values, ed->backing_size);
        named_types_[ed->name] = tid;
    }
}

void HIRBuilder::registerUnionDecls(const Module* ast) {
    for (uint32_t i = 0; i < ast->union_count; ++i) {
        auto* ud = ast->unions[i];
        if (ud->type_param_count > 0) {
            // Generic union — store as template
            generic_unions_[ud->name] = ud;
            continue;
        }
        std::vector<VariantInfo> variants;
        for (uint32_t j = 0; j < ud->variant_count; ++j) {
            TypeId payload = INVALID_TYPE;
            if (ud->variants[j].payload_type) {
                payload = resolveType(*ud->variants[j].payload_type);
            }
            variants.push_back({ctx_.strings.intern(ud->variants[j].name), payload});
        }
        TypeId tid = ctx_.types.makeUnion(ctx_.strings.intern(ud->name), variants,
                                          ud->is_repr_c);
        named_types_[ud->name] = tid;
    }
}

void HIRBuilder::registerFnSigs(const Module* ast) {
    for (uint32_t i = 0; i < ast->fn_count; ++i) {
        auto* fn = ast->functions[i];

        // Temporarily register type params for resolveType
        std::vector<std::pair<std::string_view, TypeId>> saved;
        for (uint32_t t = 0; t < fn->type_param_count; ++t) {
            auto interned = ctx_.strings.intern(fn->type_params[t].name);
            if (named_types_.count(interned)) {
                saved.push_back({interned, named_types_[interned]});
            }
            TypeInfo ti{};
            ti.kind = TypeKind::TypeVar;
            ti.type_var.name = interned;
            named_types_[interned] = ctx_.types.add(ti);
        }

        FnSig sig;
        sig.name = fn->name;
        sig.return_type = resolveType(fn->return_type);
        for (uint32_t j = 0; j < fn->param_count; ++j) {
            sig.param_types.push_back(resolveType(fn->params[j].type));
        }
        fn_table_[fn->name] = sig;

        // Restore shadowed types
        for (uint32_t t = 0; t < fn->type_param_count; ++t) {
            named_types_.erase(ctx_.strings.intern(fn->type_params[t].name));
        }
        for (auto& [name, tid] : saved) {
            named_types_[name] = tid;
        }
    }

    // Validate parameter count limit (System V ABI: 6 integer regs)
    for (uint32_t i = 0; i < ast->fn_count; ++i) {
        if (ast->functions[i]->param_count > 6) {
            ctx_.diag.error(ast->functions[i]->loc,
                std::string("function '") + std::string(ast->functions[i]->name) +
                "' has " + std::to_string(ast->functions[i]->param_count) +
                " parameters, maximum is 6 (System V ABI register limit)");
        }
    }
}

// ============================================================================
// Cross-module export injection
// ============================================================================

void HIRBuilder::registerExports(const Module* ast, std::string_view module_path) {
    if (!ast) return;

    // Determine module name: use explicit parameter, fall back to ast->module_name
    std::string_view mod_name = module_path.empty() ? ast->module_name : module_path;

    // Register type aliases
    for (uint32_t i = 0; i < ast->type_alias_count; ++i) {
        auto* ta = ast->type_aliases[i];
        if (!ta->is_pub) continue;
        TypeId target = resolveType(ta->target);
        named_types_[ta->name] = target;
    }

    // Register newtypes
    for (uint32_t i = 0; i < ast->newtype_count; ++i) {
        auto* nt = ast->newtypes[i];
        if (!nt->is_pub) continue;
        TypeId inner = resolveType(nt->inner);
        FieldInfo field = {ctx_.strings.intern("inner"), inner, false, 0};
        TypeId tid = ctx_.types.makeStruct(ctx_.strings.intern(nt->name), {&field, 1});
        named_types_[nt->name] = tid;
    }

    // Register struct types (pub only)
    for (uint32_t i = 0; i < ast->struct_count; ++i) {
        auto* sd = ast->structs[i];
        if (!sd->is_pub) continue;
        if (sd->type_param_count > 0) {
            generic_structs_[sd->name] = sd;
            continue;
        }
        std::vector<FieldInfo> fields;
        for (uint32_t j = 0; j < sd->field_count; ++j) {
            auto& f = sd->fields[j];
            TypeId ft = resolveType(f.type);
            fields.push_back({ctx_.strings.intern(f.name), ft, f.is_mutable, -1,
                              f.bit_width, 0});
        }
        TypeId tid = ctx_.types.makeStruct(ctx_.strings.intern(sd->name), fields,
                                            sd->is_packed, sd->explicit_align,
                                            sd->is_repr_c);
        named_types_[sd->name] = tid;
    }

    // Register enum types (pub only)
    for (uint32_t i = 0; i < ast->enum_count; ++i) {
        auto* ed = ast->enums[i];
        if (!ed->is_pub) continue;
        std::vector<std::string_view> names;
        std::vector<int64_t> values;
        for (uint32_t j = 0; j < ed->variant_count; ++j) {
            names.push_back(ctx_.strings.intern(ed->variants[j].name));
            values.push_back(static_cast<int64_t>(j));
        }
        TypeId tid = ctx_.types.makeEnum(ctx_.strings.intern(ed->name), names, values, ed->backing_size);
        named_types_[ed->name] = tid;
    }

    // Register union types (pub only)
    for (uint32_t i = 0; i < ast->union_count; ++i) {
        auto* ud = ast->unions[i];
        if (!ud->is_pub) continue;
        if (ud->type_param_count > 0) {
            generic_unions_[ud->name] = ud;
            continue;
        }
        std::vector<VariantInfo> variants;
        for (uint32_t j = 0; j < ud->variant_count; ++j) {
            TypeId payload = INVALID_TYPE;
            if (ud->variants[j].payload_type) {
                payload = resolveType(*ud->variants[j].payload_type);
            }
            variants.push_back({ctx_.strings.intern(ud->variants[j].name), payload});
        }
        TypeId tid = ctx_.types.makeUnion(ctx_.strings.intern(ud->name), variants,
                                          ud->is_repr_c);
        named_types_[ud->name] = tid;
    }

    // Register traits (pub only)
    for (uint32_t i = 0; i < ast->trait_count; ++i) {
        auto* td = ast->traits[i];
        if (!td->is_pub) continue;
        TraitInfo ti;
        ti.name = ctx_.strings.intern(td->name);
        for (uint32_t j = 0; j < td->method_count; ++j) {
            ti.method_names.push_back(ctx_.strings.intern(td->methods[j].name));
            ti.method_effects.push_back(EffectSet{});
        }
        trait_table_[td->name] = std::move(ti);
    }

    // Register impl methods (pub only)
    for (uint32_t i = 0; i < ast->impl_count; ++i) {
        auto* imp = ast->impls[i];
        auto type_name = ctx_.strings.intern(imp->target_type.name);
        for (uint32_t j = 0; j < imp->method_count; ++j) {
            auto* m = imp->methods[j];
            // Build mangled name
            std::string mangled_str = std::string(imp->target_type.name) + "_" + std::string(m->name);
            auto mangled = ctx_.strings.intern(mangled_str);
            impl_table_[type_name].methods[ctx_.strings.intern(m->name)] = mangled;

            // Also register the mangled fn sig
            FnSig sig;
            sig.name = mangled;
            sig.return_type = resolveType(m->return_type);
            for (uint32_t k = 0; k < m->param_count; ++k) {
                sig.param_types.push_back(resolveType(m->params[k].type));
            }
            fn_table_[mangled] = sig;
        }
    }

    // Register function signatures (pub only)
    for (uint32_t i = 0; i < ast->fn_count; ++i) {
        auto* fn = ast->functions[i];
        if (!fn->is_pub) continue;

        // Temporarily register type params for resolveType
        std::vector<std::pair<std::string_view, TypeId>> saved;
        for (uint32_t t = 0; t < fn->type_param_count; ++t) {
            auto interned = ctx_.strings.intern(fn->type_params[t].name);
            if (named_types_.count(interned)) {
                saved.push_back({interned, named_types_[interned]});
            }
            TypeInfo ti{};
            ti.kind = TypeKind::TypeVar;
            ti.type_var.name = interned;
            named_types_[interned] = ctx_.types.add(ti);
        }

        FnSig sig;
        sig.name = fn->name;
        sig.return_type = resolveType(fn->return_type);
        for (uint32_t j = 0; j < fn->param_count; ++j) {
            sig.param_types.push_back(resolveType(fn->params[j].type));
        }
        fn_table_[fn->name] = sig;

        // Track module origin for cross-module calls
        if (!mod_name.empty()) {
            fn_module_map_[fn->name] = mod_name;
        }

        // Restore shadowed types
        for (uint32_t t = 0; t < fn->type_param_count; ++t) {
            named_types_.erase(ctx_.strings.intern(fn->type_params[t].name));
        }
        for (auto& [name, tid] : saved) {
            named_types_[name] = tid;
        }
    }

    // Register global variables (pub only)
    for (uint32_t i = 0; i < ast->global_count; ++i) {
        auto* gd = ast->globals[i];
        if (!gd->is_pub) continue;
        auto name = ctx_.strings.intern(gd->name);
        TypeId tid = resolveType(gd->type);
        global_types_[name] = tid;
    }
}

void HIRBuilder::injectFnSig(std::string_view name, const std::vector<TypeId>& param_types,
                               TypeId return_type) {
    FnSig sig;
    sig.name = name;
    sig.param_types = param_types;
    sig.return_type = return_type;
    fn_table_[name] = sig;
}

void HIRBuilder::injectNamedType(std::string_view name, TypeId tid) {
    named_types_[name] = tid;
}

void HIRBuilder::injectGenericStruct(std::string_view name, const StructDecl* decl) {
    generic_structs_[name] = decl;
}

void HIRBuilder::injectGenericUnion(std::string_view name, const UnionDecl* decl) {
    generic_unions_[name] = decl;
}

void HIRBuilder::injectGlobalType(std::string_view name, TypeId tid) {
    global_types_[name] = tid;
}

// ============================================================================
// Module building
// ============================================================================

HIRModule* HIRBuilder::build(const Module* ast) {
    if (!ast) return nullptr;

    // Register type aliases first (they may be used by other types)
    for (uint32_t i = 0; i < ast->type_alias_count; ++i) {
        auto* ta = ast->type_aliases[i];
        TypeId target = resolveType(ta->target);
        named_types_[ta->name] = target;
    }

    // Register newtypes as single-field structs
    for (uint32_t i = 0; i < ast->newtype_count; ++i) {
        auto* nt = ast->newtypes[i];
        TypeId inner = resolveType(nt->inner);
        FieldInfo field = {ctx_.strings.intern("inner"), inner, false, 0};
        TypeId tid = ctx_.types.makeStruct(ctx_.strings.intern(nt->name), {&field, 1});
        named_types_[nt->name] = tid;
    }

    registerStructDecls(ast);
    registerEnumDecls(ast);
    registerUnionDecls(ast);
    registerTraits(ast);
    registerImpls(ast);
    registerFnSigs(ast);

    // Pre-register global variable types (so they're visible during fn building)
    for (uint32_t i = 0; i < ast->global_count; ++i) {
        auto* gd = ast->globals[i];
        auto name = ctx_.strings.intern(gd->name);
        TypeId tid = resolveType(gd->type);
        global_types_[name] = tid;
    }

    auto* mod = ctx_.arena.make<HIRModule>();
    mod->module_name = ast->module_name;

    // Struct declarations
    mod->struct_count = ast->struct_count;
    mod->structs = ctx_.arena.makeArray<HIRStructDecl*>(ast->struct_count);
    for (uint32_t i = 0; i < ast->struct_count; ++i) {
        auto* hsd = ctx_.arena.make<HIRStructDecl>();
        hsd->name = ctx_.strings.intern(ast->structs[i]->name);
        hsd->type_id = named_types_[ast->structs[i]->name];
        hsd->loc = ast->structs[i]->loc;
        mod->structs[i] = hsd;
    }

    // Enum declarations
    mod->enum_count = ast->enum_count;
    mod->enums = ctx_.arena.makeArray<HIREnumDecl*>(ast->enum_count);
    for (uint32_t i = 0; i < ast->enum_count; ++i) {
        auto* hed = ctx_.arena.make<HIREnumDecl>();
        hed->name = ctx_.strings.intern(ast->enums[i]->name);
        hed->type_id = named_types_[ast->enums[i]->name];
        hed->loc = ast->enums[i]->loc;
        mod->enums[i] = hed;
    }

    // Union declarations
    mod->union_count = ast->union_count;
    mod->unions = ctx_.arena.makeArray<HIRUnionDecl*>(ast->union_count);
    for (uint32_t i = 0; i < ast->union_count; ++i) {
        auto* hud = ctx_.arena.make<HIRUnionDecl>();
        hud->name = ctx_.strings.intern(ast->unions[i]->name);
        hud->type_id = named_types_[ast->unions[i]->name];
        hud->loc = ast->unions[i]->loc;
        mod->unions[i] = hud;
    }

    // Build global declarations (before functions, so globals are visible in function bodies)
    mod->global_count = ast->global_count;
    mod->globals = ctx_.arena.makeArray<HIRGlobalDecl*>(ast->global_count);
    for (uint32_t i = 0; i < ast->global_count; ++i) {
        auto* gd = ast->globals[i];
        auto* hgd = ctx_.arena.make<HIRGlobalDecl>();
        hgd->name = ctx_.strings.intern(gd->name);
        hgd->type_id = resolveType(gd->type);
        hgd->is_mutable = gd->is_mutable;
        hgd->is_extern = gd->is_extern;
        hgd->section_name = gd->section_name;
        hgd->link_name = gd->link_name;
        hgd->loc = gd->loc;
        if (gd->is_extern && gd->init) {
            ctx_.diag.error(gd->loc, "extern global '" + std::string(gd->name) +
                            "' cannot have an initializer");
        }
        if (!gd->is_extern && !gd->init) {
            ctx_.diag.error(gd->loc, "non-extern global '" + std::string(gd->name) +
                            "' must have an initializer");
        }
        if (gd->init) {
            hgd->init = buildExpr(gd->init);
        } else {
            hgd->init = nullptr;
        }
        mod->globals[i] = hgd;
        global_vars_[hgd->name] = hgd;
    }

    // Functions
    lifted_lambdas_.clear();
    lambda_counter_ = 0;

    // First pass: build all declared functions (may produce lifted lambdas)
    auto* ast_fns = ctx_.arena.makeArray<HIRFnDecl*>(ast->fn_count);
    for (uint32_t i = 0; i < ast->fn_count; ++i) {
        ast_fns[i] = buildFn(ast->functions[i]);
        hir_fns_[ast_fns[i]->name] = ast_fns[i];
    }

    // Evaluate static_assert declarations (after functions are built, so const fn calls work)
    for (uint32_t i = 0; i < ast->static_assert_count; ++i) {
        auto* sa = ast->static_asserts[i];
        HIRExpr* cond = buildExpr(sa->condition);
        int64_t val;
        if (constEvalInt(cond, &val)) {
            if (val == 0) {
                ctx_.diag.error(sa->loc, std::string("static assertion failed: ") +
                                std::string(sa->message));
            }
        } else {
            ctx_.diag.error(sa->loc, "static_assert condition must be a compile-time constant");
        }
    }

    // Build impl methods as mangled functions
    std::vector<HIRFnDecl*> impl_fns;
    for (uint32_t i = 0; i < ast->impl_count; ++i) {
        auto* id = ast->impls[i];
        TypeId target_type = resolveType(id->target_type);
        std::string_view type_name = actualTypeName(target_type, ctx_.types);
        for (uint32_t j = 0; j < id->method_count; ++j) {
            auto* fn = id->methods[j];
            auto method_name = ctx_.strings.intern(fn->name);
            std::string mangled = std::string(type_name) + "_" + std::string(method_name);
            char* buf = ctx_.arena.makeArray<char>(mangled.size());
            std::memcpy(buf, mangled.data(), mangled.size());
            auto interned_mangled = ctx_.strings.intern(std::string_view(buf, mangled.size()));

            auto original_name = fn->name;
            fn->name = interned_mangled;
            HIRFnDecl* hfn = buildFn(fn);
            fn->name = original_name;
            impl_fns.push_back(hfn);
        }
    }

    // Merge original functions + impl methods + lifted lambdas
    uint32_t total_fns = ast->fn_count +
        static_cast<uint32_t>(impl_fns.size()) +
        static_cast<uint32_t>(lifted_lambdas_.size());
    mod->fn_count = total_fns;
    mod->functions = ctx_.arena.makeArray<HIRFnDecl*>(total_fns);
    uint32_t idx = 0;
    for (uint32_t i = 0; i < ast->fn_count; ++i) {
        mod->functions[idx++] = ast_fns[i];
    }
    for (uint32_t i = 0; i < impl_fns.size(); ++i) {
        mod->functions[idx++] = impl_fns[i];
    }
    for (uint32_t i = 0; i < lifted_lambdas_.size(); ++i) {
        mod->functions[idx++] = lifted_lambdas_[i];
    }

    // Populate vtable entries from accumulated vtable_globals_
    mod->vtable_count = static_cast<uint32_t>(vtable_globals_.size());
    if (mod->vtable_count > 0) {
        mod->vtables = ctx_.arena.makeArray<HIRModule::VTableEntry>(mod->vtable_count);
        for (uint32_t i = 0; i < mod->vtable_count; ++i) {
            auto& vt = vtable_globals_[i];
            mod->vtables[i].label = vt.label;
            mod->vtables[i].method_count = static_cast<uint32_t>(vt.fn_labels.size());
            mod->vtables[i].fn_labels = ctx_.arena.makeArray<std::string_view>(vt.fn_labels.size());
            for (uint32_t j = 0; j < vt.fn_labels.size(); ++j) {
                mod->vtables[i].fn_labels[j] = vt.fn_labels[j];
            }
        }
    } else {
        mod->vtables = nullptr;
    }

    return mod;
}

// ============================================================================
// Function building
// ============================================================================

HIRFnDecl* HIRBuilder::buildFn(const FnDecl* fn) {
    local_vars_.clear();
    mutable_vars_.clear();

    // Save and register type parameters for this function
    std::vector<std::pair<std::string_view, TypeId>> saved_type_params;
    auto* hfn = ctx_.arena.make<HIRFnDecl>();
    hfn->type_param_count = fn->type_param_count;
    if (fn->type_param_count > 0) {
        hfn->type_params = ctx_.arena.makeArray<HIRTypeParam>(fn->type_param_count);
        for (uint32_t i = 0; i < fn->type_param_count; ++i) {
            auto interned = ctx_.strings.intern(fn->type_params[i].name);
            if (fn->type_params[i].is_const) {
                // Const generic: resolve the const param's type, register as const
                TypeId ct = resolveType(fn->type_params[i].const_type);
                hfn->type_params[i] = {interned, ct, true, ct};
                // Don't register in named_types_; const params live in const_values_
            } else {
                // Type generic: reuse TypeVar from registerFnSigs if available
                TypeId tv_id = INVALID_TYPE;
                auto fn_it = fn_table_.find(fn->name);
                if (fn_it != fn_table_.end()) {
                    // Deep-find TypeVar by name in all sig types (params + return)
                    std::function<TypeId(TypeId)> findTypeVar = [&](TypeId tid) -> TypeId {
                        const auto& ti2 = ctx_.types.get(tid);
                        if (ti2.kind == TypeKind::TypeVar && ti2.type_var.name == interned)
                            return tid;
                        switch (ti2.kind) {
                            case TypeKind::Union:
                                for (uint32_t v = 0; v < ti2.union_.variant_count; ++v) {
                                    if (ti2.union_.variants[v].payload_type != INVALID_TYPE) {
                                        auto r = findTypeVar(ti2.union_.variants[v].payload_type);
                                        if (r != INVALID_TYPE) return r;
                                    }
                                }
                                break;
                            case TypeKind::Struct:
                                for (uint32_t f = 0; f < ti2.struct_.field_count; ++f) {
                                    auto r = findTypeVar(ti2.struct_.fields[f].type);
                                    if (r != INVALID_TYPE) return r;
                                }
                                break;
                            case TypeKind::Ptr:
                            case TypeKind::PtrMut:
                                return findTypeVar(ti2.ptr.pointee);
                            case TypeKind::Array:
                                return findTypeVar(ti2.array.element);
                            case TypeKind::Fn:
                                for (uint32_t p = 0; p < ti2.fn.param_count; ++p) {
                                    auto r = findTypeVar(ti2.fn.params[p]);
                                    if (r != INVALID_TYPE) return r;
                                }
                                return findTypeVar(ti2.fn.return_type);
                            default:
                                break;
                        }
                        return INVALID_TYPE;
                    };

                    for (auto pt : fn_it->second.param_types) {
                        tv_id = findTypeVar(pt);
                        if (tv_id != INVALID_TYPE) break;
                    }
                    if (tv_id == INVALID_TYPE) {
                        tv_id = findTypeVar(fn_it->second.return_type);
                    }
                }
                if (tv_id == INVALID_TYPE) {
                    // Fallback: create new TypeVar
                    TypeInfo ti{};
                    ti.kind = TypeKind::TypeVar;
                    ti.type_var.name = interned;
                    tv_id = ctx_.types.add(ti);
                }
                hfn->type_params[i] = {interned, tv_id, false, INVALID_TYPE};
                if (named_types_.count(interned)) {
                    saved_type_params.push_back({interned, named_types_[interned]});
                }
                named_types_[interned] = tv_id;
            }
        }
    }

    current_return_type_ = resolveType(fn->return_type);

    hfn->name = ctx_.strings.intern(fn->name);
    hfn->param_count = fn->param_count;
    hfn->params = ctx_.arena.makeArray<HIRParam>(fn->param_count);
    hfn->return_type = current_return_type_;
    hfn->purity = 4; // Purity::Unknown
    hfn->is_recursive = false;
    hfn->is_tail_recursive = false;
    hfn->is_intrinsic = fn->is_intrinsic;
    hfn->is_const = fn->is_const;
    hfn->is_naked = fn->is_naked;
    hfn->is_interrupt = fn->is_interrupt;
    hfn->is_inline = fn->is_inline;
    hfn->is_noinline = fn->is_noinline;
    hfn->is_noreturn = fn->is_noreturn;
    hfn->is_pub = fn->is_pub || fn->name == "main";  // main is always pub
    hfn->is_extern = fn->is_extern;
    hfn->is_variadic = fn->is_variadic;
    hfn->is_weak = fn->is_weak;
    hfn->link_name = fn->link_name;
    hfn->section_name = fn->section_name;

    // Process effect annotations from "with io, atomic" clause
    EffectSet declared = EFFECT_NONE;
    hfn->has_effect_annotation = fn->has_effect_clause;
    for (uint32_t i = 0; i < fn->effect_count; ++i) {
        Effect eff;
        if (parseEffectName(fn->effect_names[i], eff)) {
            declared = addEffect(declared, eff);
        } else if (fn->effect_names[i] != "pure") {
            ctx_.diag.error(fn->loc, std::string("unknown effect '") +
                            std::string(fn->effect_names[i]) + "'");
        }
        // "pure" is a valid annotation that means EFFECT_NONE (no error)
    }
    hfn->declared_effects = declared;
    hfn->inferred_effects = EFFECT_NONE;

    hfn->loc = fn->loc;

    // Register parameters
    for (uint32_t i = 0; i < fn->param_count; ++i) {
        TypeId pt = resolveType(fn->params[i].type);
        auto interned_name = ctx_.strings.intern(fn->params[i].name);
        hfn->params[i] = {interned_name, pt, fn->params[i].loc,
                          static_cast<uint8_t>(fn->params[i].mode)};
        local_vars_[interned_name] = pt;
    }

    auto cleanup_type_params = [&]() {
        // Restore shadowed named types and const values
        for (uint32_t i = 0; i < fn->type_param_count; ++i) {
            auto interned = hfn->type_params[i].name;
            if (hfn->type_params[i].is_const) {
                const_values_.erase(interned);
            } else {
                named_types_.erase(interned);
            }
        }
        for (auto& [name, tid] : saved_type_params) {
            named_types_[name] = tid;
        }
    };

    if (fn->is_intrinsic || fn->is_extern) {
        hfn->body = nullptr;
        cleanup_type_params();
        return hfn;
    }

    // Skip body building for generic functions — they'll be monomorphized later
    if (fn->type_param_count > 0) {
        hfn->body = buildExpr(fn->body, current_return_type_);
        cleanup_type_params();
        return hfn;
    }

    // Build body
    hfn->body = buildExpr(fn->body, current_return_type_);

    // Check return type match (skip for naked/interrupt — user controls return via asm)
    if (!hfn->is_naked && !hfn->is_interrupt && hfn->body && hfn->body->type != TypeTable::Error &&
        !typesMatchClosure(hfn->body->type, current_return_type_, ctx_.types, closure_struct_types_)) {
        hfn->body = implicitWiden(hfn->body, current_return_type_);
        if (!typesMatchClosure(hfn->body->type, current_return_type_, ctx_.types, closure_struct_types_)) {
            ctx_.diag.error(fn->loc, std::string("function '") + std::string(fn->name) +
                        "' declared to return " + ctx_.types.name(current_return_type_) +
                        " but body has type " + ctx_.types.name(hfn->body->type));
        }
    }

    // If the body returns a closure struct and the declared return is a Fn type,
    // promote the function's return type to the closure struct type so callers
    // can unpack the closure properly.
    if (hfn->body && closure_struct_types_.count(hfn->body->type) &&
        current_return_type_ < ctx_.types.size() &&
        ctx_.types.get(current_return_type_).kind == TypeKind::Fn) {
        hfn->return_type = hfn->body->type;
        // Also update the fn_table_ entry so callers see the closure struct type
        auto fn_it = fn_table_.find(ctx_.strings.intern(fn->name));
        if (fn_it != fn_table_.end()) {
            fn_it->second.return_type = hfn->body->type;
        }
    }

    cleanup_type_params();
    return hfn;
}

// ============================================================================
// Expression building
// ============================================================================

HIRExpr* HIRBuilder::buildExpr(const Expr* expr, std::optional<TypeId> ctx_type) {
    if (!expr) return nullptr;

    switch (expr->kind) {
        case Expr::Kind::IntLit:      return buildIntLit(expr, ctx_type);
        case Expr::Kind::FloatLit:    return buildFloatLit(expr, ctx_type);
        case Expr::Kind::BoolLit:     return buildBoolLit(expr);
        case Expr::Kind::NullLit: {
            auto* e = ctx_.arena.make<HIRIntLitExpr>();
            e->kind = HIRExpr::Kind::IntLit;
            e->loc = expr->loc;
            e->value = 0;
            if (ctx_type && isPtrType(*ctx_type, ctx_.types)) {
                e->type = *ctx_type;
            } else {
                e->type = ctx_.types.makePtr(TypeTable::Unit, false);
            }
            return e;
        }
        case Expr::Kind::StringLit:   return buildStringLit(expr);
        case Expr::Kind::Ident:       return buildIdent(expr);
        case Expr::Kind::BinOp:       return buildBinOp(expr, ctx_type);
        case Expr::Kind::UnaryOp:     return buildUnaryOp(expr);
        case Expr::Kind::Call:        return buildCall(expr);
        case Expr::Kind::If:          return buildIf(expr, ctx_type);
        case Expr::Kind::Match:       return buildMatch(expr, ctx_type);
        case Expr::Kind::Block:       return buildBlock(expr, ctx_type);
        case Expr::Kind::Return:      return buildReturn(expr);
        case Expr::Kind::StructLit:   return buildStructLit(expr);
        case Expr::Kind::FieldAccess: return buildFieldAccess(expr);
        case Expr::Kind::EnumAccess:  return buildEnumAccess(expr);
        case Expr::Kind::UnionVariant:return buildUnionVariant(expr);
        case Expr::Kind::Cast:     return buildCast(expr);
        case Expr::Kind::Loop:       return buildLoop(expr, ctx_type);
        case Expr::Kind::ForRange:   return buildForRange(expr);
        case Expr::Kind::ForEach:    return buildForEach(expr);
        case Expr::Kind::WhileLoop:  return buildWhileLoop(expr);
        case Expr::Kind::InlineAsm:  return buildInlineAsm(expr);
        case Expr::Kind::ArrayLit:   return buildArrayLit(expr);
        case Expr::Kind::IndexAccess:return buildIndexAccess(expr);
        case Expr::Kind::SliceExpr:  return buildSliceExpr(expr);
        case Expr::Kind::Sizeof: {
            auto* se = static_cast<const SizeofExpr*>(expr);
            TypeId tid = resolveType(se->target);
            auto* e = ctx_.arena.make<HIRIntLitExpr>();
            e->kind = HIRExpr::Kind::IntLit;
            e->loc = expr->loc;
            e->value = static_cast<int64_t>(ctx_.types.sizeOf(tid));
            e->type = TypeTable::U64;
            return e;
        }
        case Expr::Kind::Alignof: {
            auto* ae = static_cast<const AlignofExpr*>(expr);
            TypeId tid = resolveType(ae->target);
            auto* e = ctx_.arena.make<HIRIntLitExpr>();
            e->kind = HIRExpr::Kind::IntLit;
            e->loc = expr->loc;
            e->value = static_cast<int64_t>(ctx_.types.alignOf(tid));
            e->type = TypeTable::U64;
            return e;
        }
        case Expr::Kind::Offsetof: {
            auto* oe = static_cast<const OffsetofExpr*>(expr);
            TypeId tid = resolveType(oe->target);
            int32_t off = ctx_.types.offsetOf(tid, oe->field_name);
            if (off < 0) {
                ctx_.diag.error(expr->loc,
                    std::string("type '") + ctx_.types.name(tid) +
                    "' has no field '" + std::string(oe->field_name) + "'");
                off = 0;
            }
            auto* e = ctx_.arena.make<HIRIntLitExpr>();
            e->kind = HIRExpr::Kind::IntLit;
            e->loc = expr->loc;
            e->value = static_cast<int64_t>(off);
            e->type = TypeTable::U64;
            return e;
        }
        case Expr::Kind::Lambda:
            return buildLambda(expr, ctx_type);
        case Expr::Kind::MethodCall:
            return buildMethodCall(expr);
        case Expr::Kind::Try:
            return buildTry(expr);
    }
    return errorExpr(expr->loc);
}

HIRExpr* HIRBuilder::buildIntLit(const Expr* expr, std::optional<TypeId> ctx_type) {
    auto* lit = static_cast<const IntLitExpr*>(expr);
    auto* e = ctx_.arena.make<HIRIntLitExpr>();
    e->kind = HIRExpr::Kind::IntLit;
    e->loc = expr->loc;
    e->value = lit->value;

    // If literal has an explicit type suffix, use that type
    if (!lit->suffix.empty()) {
        TypeId suffix_type = TypeTable::I64;
        if (lit->suffix == "u8")  suffix_type = TypeTable::U8;
        else if (lit->suffix == "u16") suffix_type = TypeTable::U16;
        else if (lit->suffix == "u32") suffix_type = TypeTable::U32;
        else if (lit->suffix == "u64") suffix_type = TypeTable::U64;
        else if (lit->suffix == "i8")  suffix_type = TypeTable::I8;
        else if (lit->suffix == "i16") suffix_type = TypeTable::I16;
        else if (lit->suffix == "i32") suffix_type = TypeTable::I32;
        else if (lit->suffix == "i64") suffix_type = TypeTable::I64;
        if (intFitsInType(lit->value, suffix_type)) {
            e->type = suffix_type;
        } else {
            ctx_.diag.error(expr->loc,
                std::string("integer literal ") + std::to_string(lit->value) +
                " is out of range for type " + std::string(lit->suffix));
            e->type = TypeTable::Error;
        }
        return e;
    }

    if (ctx_type && isIntegerType(*ctx_type)) {
        if (intFitsInType(lit->value, *ctx_type)) {
            e->type = *ctx_type;
        } else {
            ctx_.diag.error(expr->loc,
                std::string("integer literal ") + std::to_string(lit->value) +
                " is out of range for type " + ctx_.types.name(*ctx_type));
            e->type = TypeTable::Error;
        }
    } else {
        e->type = TypeTable::I64;
    }
    return e;
}

HIRExpr* HIRBuilder::buildFloatLit(const Expr* expr, std::optional<TypeId> ctx_type) {
    auto* fl = static_cast<const FloatLitExpr*>(expr);
    auto* e = ctx_.arena.make<HIRFloatLitExpr>();
    e->kind = HIRExpr::Kind::FloatLit;
    e->loc = expr->loc;
    e->value = fl->value;

    if (ctx_type && isFloatType(*ctx_type)) {
        e->type = *ctx_type;
    } else {
        e->type = fl->is_f32 ? TypeTable::F32 : TypeTable::F64;
    }
    return e;
}

HIRExpr* HIRBuilder::buildBoolLit(const Expr* expr) {
    auto* bl = static_cast<const BoolLitExpr*>(expr);
    auto* e = ctx_.arena.make<HIRBoolLitExpr>();
    e->kind = HIRExpr::Kind::BoolLit;
    e->loc = expr->loc;
    e->type = TypeTable::Bool;
    e->value = bl->value;
    return e;
}

HIRExpr* HIRBuilder::buildStringLit(const Expr* expr) {
    auto* sl = static_cast<const StringLitExpr*>(expr);
    auto* e = ctx_.arena.make<HIRStringLitExpr>();
    e->kind = HIRExpr::Kind::StringLit;
    e->loc = expr->loc;
    e->data = sl->data;
    e->length = sl->length;

    // String TypeId — look up or register
    auto it = named_types_.find("String");
    if (it != named_types_.end()) {
        e->type = it->second;
    } else {
        TypeRef ref;
        ref.kind = TypeRef::Kind::Named;
        ref.name = "String";
        ref.loc = expr->loc;
        e->type = resolveType(ref);
    }
    return e;
}

HIRExpr* HIRBuilder::buildIdent(const Expr* expr) {
    auto* ident = static_cast<const IdentExpr*>(expr);

    auto it = local_vars_.find(ident->name);
    if (it != local_vars_.end()) {
        auto* e = ctx_.arena.make<HIRIdentExpr>();
        e->kind = HIRExpr::Kind::Ident;
        e->loc = expr->loc;
        e->name = ctx_.strings.intern(ident->name);
        e->type = it->second;
        return e;
    }

    // Inside a lambda: check outer scope for capture
    if (in_lambda_) {
        auto outer_it = outer_locals_.find(ident->name);
        if (outer_it != outer_locals_.end()) {
            auto interned = ctx_.strings.intern(ident->name);
            // Record this capture (avoid duplicates)
            bool already_captured = false;
            for (auto& cap : current_captures_) {
                if (cap.name == interned) { already_captured = true; break; }
            }
            if (!already_captured) {
                current_captures_.push_back({interned, outer_it->second,
                    outer_mutables_.count(ident->name) > 0});
            }
            // Add to local scope so the variable is visible for the rest of the body
            local_vars_[interned] = outer_it->second;
            if (outer_mutables_.count(ident->name)) mutable_vars_.insert(interned);

            auto* e = ctx_.arena.make<HIRIdentExpr>();
            e->kind = HIRExpr::Kind::Ident;
            e->loc = expr->loc;
            e->name = interned;
            e->type = outer_it->second;
            return e;
        }
    }

    // Check if it's a global variable
    auto gv_it = global_types_.find(ident->name);
    if (gv_it != global_types_.end()) {
        auto* e = ctx_.arena.make<HIRIdentExpr>();
        e->kind = HIRExpr::Kind::Ident;
        e->loc = expr->loc;
        e->name = ctx_.strings.intern(ident->name);
        e->type = gv_it->second;
        return e;
    }

    // Check if it's a function name → FnRef (function pointer)
    auto fn_it = fn_table_.find(ident->name);
    if (fn_it != fn_table_.end()) {
        auto& sig = fn_it->second;
        // Build function type: fn(param_types...) -> return_type
        std::vector<TypeId> param_types(sig.param_types.begin(), sig.param_types.end());
        TypeId fn_type = ctx_.types.makeFn(param_types, sig.return_type);

        auto* e = ctx_.arena.make<HIRFnRefExpr>();
        e->kind = HIRExpr::Kind::FnRef;
        e->loc = expr->loc;
        e->fn_name = ctx_.strings.intern(ident->name);
        e->type = fn_type;
        return e;
    }

    auto* e = ctx_.arena.make<HIRIdentExpr>();
    e->kind = HIRExpr::Kind::Ident;
    e->loc = expr->loc;
    e->name = ctx_.strings.intern(ident->name);
    ctx_.diag.error(expr->loc, std::string("undeclared identifier '") +
                std::string(ident->name) + "'");
    e->type = TypeTable::Error;
    return e;
}

static HIRBinOp convertBinOp(BinOpKind op) {
    switch (op) {
        case BinOpKind::Add:    return HIRBinOp::Add;
        case BinOpKind::Sub:    return HIRBinOp::Sub;
        case BinOpKind::Mul:    return HIRBinOp::Mul;
        case BinOpKind::Div:    return HIRBinOp::Div;
        case BinOpKind::Mod:    return HIRBinOp::Mod;
        case BinOpKind::Eq:     return HIRBinOp::Eq;
        case BinOpKind::NotEq:  return HIRBinOp::NotEq;
        case BinOpKind::Lt:     return HIRBinOp::Lt;
        case BinOpKind::LtEq:   return HIRBinOp::LtEq;
        case BinOpKind::Gt:     return HIRBinOp::Gt;
        case BinOpKind::GtEq:   return HIRBinOp::GtEq;
        case BinOpKind::And:    return HIRBinOp::And;
        case BinOpKind::Or:     return HIRBinOp::Or;
        case BinOpKind::BitAnd: return HIRBinOp::BitAnd;
        case BinOpKind::BitOr:  return HIRBinOp::BitOr;
        case BinOpKind::BitXor: return HIRBinOp::BitXor;
        case BinOpKind::Shl:    return HIRBinOp::Shl;
        case BinOpKind::Shr:    return HIRBinOp::Shr;
        case BinOpKind::AddWrap: return HIRBinOp::AddWrap;
        case BinOpKind::SubWrap: return HIRBinOp::SubWrap;
        case BinOpKind::MulWrap: return HIRBinOp::MulWrap;
        case BinOpKind::AddSat:  return HIRBinOp::AddSat;
        case BinOpKind::SubSat:  return HIRBinOp::SubSat;
    }
    return HIRBinOp::Add;
}

static std::string_view opMethodName(BinOpKind op) {
    switch (op) {
        case BinOpKind::Add: return "add";
        case BinOpKind::Sub: return "sub";
        case BinOpKind::Mul: return "mul";
        case BinOpKind::Div: return "div";
        case BinOpKind::Mod: return "mod";
        case BinOpKind::BitAnd: return "bitand";
        case BinOpKind::BitOr: return "bitor";
        case BinOpKind::BitXor: return "bitxor";
        case BinOpKind::Shl: return "shl";
        case BinOpKind::Shr: return "shr";
        case BinOpKind::Eq: return "eq";
        case BinOpKind::NotEq: return "ne";
        case BinOpKind::Lt: return "lt";
        case BinOpKind::LtEq: return "le";
        case BinOpKind::Gt: return "gt";
        case BinOpKind::GtEq: return "ge";
        default: return {};
    }
}

HIRExpr* HIRBuilder::buildBinOp(const Expr* expr, std::optional<TypeId> ctx_type) {
    auto* bin = static_cast<const BinOpExpr*>(expr);
    auto* e = ctx_.arena.make<HIRBinOpExpr>();
    e->kind = HIRExpr::Kind::BinOp;
    e->loc = expr->loc;
    e->op = convertBinOp(bin->op);

    // Context propagation
    std::optional<TypeId> operand_ctx = std::nullopt;
    bool is_cmp = false;
    switch (bin->op) {
        case BinOpKind::Add: case BinOpKind::Sub:
        case BinOpKind::Mul: case BinOpKind::Div:
        case BinOpKind::Mod:
        case BinOpKind::AddWrap: case BinOpKind::SubWrap:
        case BinOpKind::MulWrap:
        case BinOpKind::AddSat: case BinOpKind::SubSat:
        case BinOpKind::BitAnd: case BinOpKind::BitOr:
        case BinOpKind::BitXor: case BinOpKind::Shl:
        case BinOpKind::Shr:
            operand_ctx = ctx_type;
            break;
        case BinOpKind::Eq:  case BinOpKind::NotEq:
        case BinOpKind::Lt:  case BinOpKind::LtEq:
        case BinOpKind::Gt:  case BinOpKind::GtEq:
            is_cmp = true;
            break;
        case BinOpKind::And: case BinOpKind::Or:
            break;
    }

    e->lhs = buildExpr(bin->lhs, operand_ctx);
    TypeId lhs_type = e->lhs->type;

    // For comparisons, use lhs type as context for rhs
    e->rhs = buildExpr(bin->rhs, is_cmp ? std::optional<TypeId>(lhs_type) : operand_ctx);
    TypeId rhs_type = e->rhs->type;

    if (lhs_type == TypeTable::Error || rhs_type == TypeTable::Error) {
        e->type = TypeTable::Error;
        return e;
    }

    // TypeVar operands: defer validation to monomorphization
    bool lhs_is_typevar = lhs_type < ctx_.types.size() &&
                          ctx_.types.get(lhs_type).kind == TypeKind::TypeVar;
    bool rhs_is_typevar = rhs_type < ctx_.types.size() &&
                          ctx_.types.get(rhs_type).kind == TypeKind::TypeVar;
    if (lhs_is_typevar || rhs_is_typevar) {
        // Propagate TypeVar: if either side is TypeVar, result is TypeVar
        // For comparisons, result is always Bool
        bool is_cmp_op = bin->op == BinOpKind::Eq || bin->op == BinOpKind::NotEq ||
                         bin->op == BinOpKind::Lt || bin->op == BinOpKind::LtEq ||
                         bin->op == BinOpKind::Gt || bin->op == BinOpKind::GtEq;
        if (is_cmp_op) {
            e->type = TypeTable::Bool;
        } else if (lhs_is_typevar) {
            e->type = lhs_type;
        } else {
            e->type = rhs_type;
        }
        return e;
    }

    // Implicit integer widening for binary operations: widen narrower to wider
    if (lhs_type != rhs_type && isIntegerType(lhs_type) && isIntegerType(rhs_type)) {
        if (canImplicitlyWiden(lhs_type, rhs_type)) {
            e->lhs = implicitWiden(e->lhs, rhs_type);
            lhs_type = e->lhs->type;
        } else if (canImplicitlyWiden(rhs_type, lhs_type)) {
            e->rhs = implicitWiden(e->rhs, lhs_type);
            rhs_type = e->rhs->type;
        }
    }

    // Operator overloading: check if lhs type has an impl method for this op
    if (!isIntegerType(lhs_type) && !isFloatType(lhs_type) &&
        !isPtrType(lhs_type, ctx_.types) && lhs_type != TypeTable::Bool &&
        lhs_type != TypeTable::Error && lhs_type < ctx_.types.size()) {
        auto method = opMethodName(bin->op);
        if (!method.empty()) {
            auto mangled = resolveMethod(lhs_type, method);
            if (!mangled.empty()) {
                // Desugar: a + b → Type.add(a, b)
                auto fn_it = fn_table_.find(mangled);
                if (fn_it != fn_table_.end()) {
                    auto* call = ctx_.arena.make<HIRCallExpr>();
                    call->kind = HIRExpr::Kind::Call;
                    call->loc = expr->loc;
                    call->callee = mangled;
                    call->is_tail_call = false;
                    call->arg_count = 2;
                    call->args = ctx_.arena.makeArray<HIRExpr*>(2);
                    call->args[0] = e->lhs;
                    call->args[1] = e->rhs;
                    call->type = fn_it->second.return_type;
                    return call;
                }
            }
        }
    }

    switch (bin->op) {
        case BinOpKind::Add: case BinOpKind::Sub:
            // Pointer arithmetic: Ptr<T> + int → Ptr<T>
            if (isPtrType(lhs_type, ctx_.types) && isIntegerType(rhs_type)) {
                e->type = lhs_type;
                break;
            }
            if (isIntegerType(lhs_type) && isPtrType(rhs_type, ctx_.types) &&
                bin->op == BinOpKind::Add) {
                e->type = rhs_type; // int + Ptr<T> → Ptr<T>
                break;
            }
            [[fallthrough]];
        case BinOpKind::Mul: case BinOpKind::Div:
            if (!(isIntegerType(lhs_type) || isFloatType(lhs_type)) || lhs_type != rhs_type) {
                ctx_.diag.error(expr->loc,
                    std::string("arithmetic operators require same numeric type operands, got ") +
                    ctx_.types.name(lhs_type) + " and " + ctx_.types.name(rhs_type));
                e->type = TypeTable::Error;
            } else {
                e->type = lhs_type;
            }
            break;

        case BinOpKind::AddWrap: case BinOpKind::SubWrap:
        case BinOpKind::MulWrap:
        case BinOpKind::AddSat: case BinOpKind::SubSat:
            if (!isIntegerType(lhs_type) || lhs_type != rhs_type) {
                ctx_.diag.error(expr->loc,
                    std::string("wrapping/saturating operators require same integer type operands, got ") +
                    ctx_.types.name(lhs_type) + " and " + ctx_.types.name(rhs_type));
                e->type = TypeTable::Error;
            } else {
                e->type = lhs_type;
            }
            break;

        case BinOpKind::Mod:
            if (!isIntegerType(lhs_type) || lhs_type != rhs_type) {
                ctx_.diag.error(expr->loc,
                    std::string("'%' requires same integer type operands, got ") +
                    ctx_.types.name(lhs_type) + " and " + ctx_.types.name(rhs_type));
                e->type = TypeTable::Error;
            } else {
                e->type = lhs_type;
            }
            break;

        case BinOpKind::BitAnd: case BinOpKind::BitOr:
        case BinOpKind::BitXor: case BinOpKind::Shl:
        case BinOpKind::Shr:
            if (!isIntegerType(lhs_type) || lhs_type != rhs_type) {
                ctx_.diag.error(expr->loc,
                    std::string("bitwise operators require same integer type operands, got ") +
                    ctx_.types.name(lhs_type) + " and " + ctx_.types.name(rhs_type));
                e->type = TypeTable::Error;
            } else {
                e->type = lhs_type;
            }
            break;

        case BinOpKind::Eq: case BinOpKind::NotEq:
        case BinOpKind::Lt: case BinOpKind::LtEq:
        case BinOpKind::Gt: case BinOpKind::GtEq:
            if (lhs_type != rhs_type) {
                // Allow comparing different pointer types (e.g., null check)
                if (isPtrType(lhs_type, ctx_.types) && isPtrType(rhs_type, ctx_.types)) {
                    e->type = TypeTable::Bool;
                } else {
                    ctx_.diag.error(expr->loc, "comparison operators require same-type operands");
                    e->type = TypeTable::Error;
                }
            } else {
                e->type = TypeTable::Bool;
            }
            break;

        case BinOpKind::And: case BinOpKind::Or:
            if (lhs_type != TypeTable::Bool || rhs_type != TypeTable::Bool) {
                ctx_.diag.error(expr->loc, "'and'/'or' require bool operands");
                e->type = TypeTable::Error;
            } else {
                e->type = TypeTable::Bool;
            }
            break;
    }
    return e;
}

HIRExpr* HIRBuilder::buildUnaryOp(const Expr* expr) {
    auto* unary = static_cast<const UnaryOpExpr*>(expr);

    switch (unary->op) {
        case UnaryOpKind_t::AddrOf: {
            auto* e = ctx_.arena.make<HIRAddrOfExpr>();
            e->kind = HIRExpr::Kind::AddrOf;
            e->loc = expr->loc;
            e->is_mutable = false;
            e->operand = buildExpr(unary->operand);
            if (e->operand->type == TypeTable::Error) {
                e->type = TypeTable::Error;
            } else {
                e->type = ctx_.types.makePtr(e->operand->type, false);
            }
            return e;
        }
        case UnaryOpKind_t::AddrOfVar: {
            // Validate mutable binding
            if (unary->operand->kind != Expr::Kind::Ident) {
                ctx_.diag.error(expr->loc, "'&var' requires a variable name");
                return errorExpr(expr->loc);
            }
            auto* ident = static_cast<const IdentExpr*>(unary->operand);
            if (mutable_vars_.find(ident->name) == mutable_vars_.end()) {
                ctx_.diag.error(expr->loc,
                    std::string("'&var' requires a 'var' binding, but '") +
                    std::string(ident->name) + "' is immutable");
                return errorExpr(expr->loc);
            }

            auto* e = ctx_.arena.make<HIRAddrOfExpr>();
            e->kind = HIRExpr::Kind::AddrOf;
            e->loc = expr->loc;
            e->is_mutable = true;
            e->operand = buildExpr(unary->operand);
            if (e->operand->type == TypeTable::Error) {
                e->type = TypeTable::Error;
            } else {
                e->type = ctx_.types.makePtr(e->operand->type, true);
            }
            return e;
        }
        case UnaryOpKind_t::Deref: {
            auto* e = ctx_.arena.make<HIRDerefExpr>();
            e->kind = HIRExpr::Kind::Deref;
            e->loc = expr->loc;
            e->operand = buildExpr(unary->operand);
            TypeId op_type = e->operand->type;

            if (op_type == TypeTable::Error) {
                e->type = TypeTable::Error;
            } else {
                auto& info = ctx_.types.get(op_type);
                if (info.kind == TypeKind::Ptr || info.kind == TypeKind::PtrMut) {
                    e->type = info.ptr.pointee;
                } else {
                    ctx_.diag.error(expr->loc,
                        std::string("cannot dereference non-pointer type '") +
                        ctx_.types.name(op_type) + "'");
                    e->type = TypeTable::Error;
                }
            }
            return e;
        }
        default: break;
    }

    // Neg, Not, BitNot
    auto* e = ctx_.arena.make<HIRUnaryOpExpr>();
    e->kind = HIRExpr::Kind::UnaryOp;
    e->loc = expr->loc;
    if (unary->op == UnaryOpKind_t::Neg)
        e->op = HIRUnaryOp::Neg;
    else if (unary->op == UnaryOpKind_t::BitNot)
        e->op = HIRUnaryOp::BitNot;
    else
        e->op = HIRUnaryOp::Not;
    e->operand = buildExpr(unary->operand,
        (unary->op == UnaryOpKind_t::Neg) ? std::nullopt : std::nullopt);

    TypeId op_type = e->operand->type;
    if (op_type == TypeTable::Error) {
        e->type = TypeTable::Error;
    } else if (unary->op == UnaryOpKind_t::BitNot) {
        if (!isIntegerType(op_type)) {
            ctx_.diag.error(expr->loc,
                std::string("bitwise '~' requires integer operand, got ") +
                ctx_.types.name(op_type));
            e->type = TypeTable::Error;
        } else {
            e->type = op_type;
        }
    } else if (unary->op == UnaryOpKind_t::Neg) {
        if (!isSignedType(op_type) && !isFloatType(op_type)) {
            ctx_.diag.error(expr->loc,
                std::string("unary '-' requires signed integer or float operand, got ") +
                ctx_.types.name(op_type));
            e->type = TypeTable::Error;
        } else {
            e->type = op_type;
        }
    } else { // Not
        if (op_type != TypeTable::Bool) {
            ctx_.diag.error(expr->loc, "'not' requires bool operand");
            e->type = TypeTable::Error;
        } else {
            e->type = TypeTable::Bool;
        }
    }
    return e;
}

HIRExpr* HIRBuilder::buildCall(const Expr* expr) {
    auto* call = static_cast<const CallExpr*>(expr);

    // Builtin cast functions: truncate<T>(x), clamp<T>(x)
    if ((call->callee == "truncate" || call->callee == "clamp") &&
        call->type_arg_count == 1 && call->arg_count == 1) {
        TypeId target = resolveType(call->type_args[0]);
        if (target == TypeTable::Error) return errorExpr(expr->loc);
        HIRExpr* operand = buildExpr(call->args[0]);
        if (operand->type == TypeTable::Error) return errorExpr(expr->loc);

        if (!isIntegerType(operand->type) || !isIntegerType(target)) {
            ctx_.diag.error(expr->loc, std::string("'") + std::string(call->callee) +
                "' requires integer types");
            return errorExpr(expr->loc);
        }

        if (call->callee == "truncate") {
            // Simple cast — no warning, explicit intent
            auto* e = ctx_.arena.make<HIRCastExpr>();
            e->kind = HIRExpr::Kind::Cast;
            e->loc = expr->loc;
            e->type = target;
            e->operand = operand;
            e->target_type = target;
            e->is_explicit_truncate = true;
            return e;
        }

        if (call->callee == "clamp") {
            // Saturating cast: emit as truncating cast (documents intent;
            // full clamping logic is a future enhancement)
            auto* e = ctx_.arena.make<HIRCastExpr>();
            e->kind = HIRExpr::Kind::Cast;
            e->loc = expr->loc;
            e->type = target;
            e->operand = operand;
            e->target_type = target;
            e->is_explicit_truncate = true;
            return e;
        }
    }

    // Check if callee is a local variable (indirect call through function pointer)
    auto local_it = local_vars_.find(call->callee);
    if (local_it != local_vars_.end()) {
        TypeId var_type = local_it->second;

        // Check if this variable holds a closure struct (capturing closure)
        if (closure_struct_types_.count(var_type)) {
            auto& si = ctx_.types.get(var_type);
            // Closure struct layout: { __fn: fn_ptr, cap1: T1, cap2: T2, ... }
            // Extract the __fn field type (should be a Fn type)
            TypeId fn_field_type = si.struct_.fields[0].type;
            auto& fn_ti = ctx_.types.get(fn_field_type);

            // Always use indirect call through __fn field. This is correct
            // even when a variable might hold different closure types (e.g.
            // from different match arms that each create a closure).
            auto callee_name = ctx_.strings.intern(call->callee);
            uint32_t cap_count = si.struct_.field_count - 1;
            uint32_t total_args = call->arg_count + cap_count;

            // Build callee: closure.__fn
            auto* obj = ctx_.arena.make<HIRIdentExpr>();
            obj->kind = HIRExpr::Kind::Ident;
            obj->loc = expr->loc;
            obj->name = callee_name;
            obj->type = var_type;
            auto* fn_access = ctx_.arena.make<HIRFieldAccessExpr>();
            fn_access->kind = HIRExpr::Kind::FieldAccess;
            fn_access->loc = expr->loc;
            fn_access->object = obj;
            fn_access->field_name = ctx_.strings.intern("__fn");
            fn_access->type = fn_field_type;

            auto* e = ctx_.arena.make<HIRCallIndirectExpr>();
            e->kind = HIRExpr::Kind::CallIndirect;
            e->loc = expr->loc;
            e->is_tail_call = false;
            e->callee = fn_access;
            e->arg_count = total_args;
            e->args = ctx_.arena.makeArray<HIRExpr*>(total_args);
            // User-supplied args
            for (uint32_t i = 0; i < call->arg_count; ++i) {
                TypeId expected = (i < fn_ti.fn.param_count) ? fn_ti.fn.params[i] : INVALID_TYPE;
                e->args[i] = buildExpr(call->args[i], expected);
            }
            // Capture args: extract from the closure struct fields
            for (uint32_t i = 0; i < cap_count; ++i) {
                auto* cap_obj = ctx_.arena.make<HIRIdentExpr>();
                cap_obj->kind = HIRExpr::Kind::Ident;
                cap_obj->loc = expr->loc;
                cap_obj->name = callee_name;
                cap_obj->type = var_type;
                auto* fa = ctx_.arena.make<HIRFieldAccessExpr>();
                fa->kind = HIRExpr::Kind::FieldAccess;
                fa->loc = expr->loc;
                fa->object = cap_obj;
                fa->field_name = si.struct_.fields[1 + i].name;
                fa->type = si.struct_.fields[1 + i].type;
                e->args[call->arg_count + i] = fa;
            }
            e->type = fn_ti.fn.return_type;
            return e;
        }

        if (var_type < ctx_.types.size() && ctx_.types.get(var_type).kind == TypeKind::Fn) {
            auto& ti = ctx_.types.get(var_type);

            // Check if this local holds a lambda with captures → rewrite as direct call
            auto lam_it = local_lambda_map_.find(ctx_.strings.intern(call->callee));
            if (lam_it != local_lambda_map_.end()) {
                auto& lambda_name = lam_it->second;
                auto cap_it = lambda_captures_.find(lambda_name);
                if (cap_it != lambda_captures_.end() && !cap_it->second.empty()) {
                    auto& caps = cap_it->second;
                    // Rewrite as direct call to lifted function with capture args appended
                    auto* e = ctx_.arena.make<HIRCallExpr>();
                    e->kind = HIRExpr::Kind::Call;
                    e->loc = expr->loc;
                    e->callee = lambda_name;
                    e->is_tail_call = false;
                    e->type_args = nullptr;
                    e->type_arg_count = 0;

                    uint32_t total_args = call->arg_count + static_cast<uint32_t>(caps.size());
                    e->arg_count = total_args;
                    e->args = ctx_.arena.makeArray<HIRExpr*>(total_args);

                    // User-supplied args
                    for (uint32_t i = 0; i < call->arg_count; ++i) {
                        TypeId expected = (i < ti.fn.param_count) ? ti.fn.params[i] : INVALID_TYPE;
                        e->args[i] = buildExpr(call->args[i], expected);
                    }
                    // Capture args — emit ident refs to the captured variables
                    for (uint32_t i = 0; i < caps.size(); ++i) {
                        auto* cap_ref = ctx_.arena.make<HIRIdentExpr>();
                        cap_ref->kind = HIRExpr::Kind::Ident;
                        cap_ref->loc = expr->loc;
                        cap_ref->name = caps[i].name;
                        cap_ref->type = caps[i].type;
                        e->args[call->arg_count + i] = cap_ref;
                    }
                    e->type = ti.fn.return_type;
                    return e;
                }
            }

            // Normal indirect call (no captures)
            auto* e = ctx_.arena.make<HIRCallIndirectExpr>();
            e->kind = HIRExpr::Kind::CallIndirect;
            e->loc = expr->loc;
            e->is_tail_call = false;

            // Build the callee expression (load the variable)
            auto* callee_expr = ctx_.arena.make<HIRIdentExpr>();
            callee_expr->kind = HIRExpr::Kind::Ident;
            callee_expr->loc = expr->loc;
            callee_expr->name = ctx_.strings.intern(call->callee);
            callee_expr->type = var_type;
            e->callee = callee_expr;

            if (call->arg_count != ti.fn.param_count) {
                ctx_.diag.error(expr->loc, std::string("function pointer expects ") +
                    std::to_string(ti.fn.param_count) + " arguments, got " +
                    std::to_string(call->arg_count));
                e->type = TypeTable::Error;
                e->args = nullptr;
                e->arg_count = 0;
                return e;
            }

            e->arg_count = call->arg_count;
            e->args = ctx_.arena.makeArray<HIRExpr*>(call->arg_count);
            for (uint32_t i = 0; i < call->arg_count; ++i) {
                TypeId expected = ti.fn.params[i];
                e->args[i] = buildExpr(call->args[i], expected);
                if (e->args[i]->type != TypeTable::Error && e->args[i]->type != expected) {
                    if (typesMatchClosure(e->args[i]->type, expected, ctx_.types, closure_struct_types_)) {
                        // Closure struct passed where Fn expected: extract __fn field
                        if (closure_struct_types_.count(e->args[i]->type) &&
                            expected < ctx_.types.size() && ctx_.types.get(expected).kind == TypeKind::Fn) {
                            auto* access = ctx_.arena.make<HIRFieldAccessExpr>();
                            access->kind = HIRExpr::Kind::FieldAccess;
                            access->loc = call->args[i]->loc;
                            access->object = e->args[i];
                            access->field_name = ctx_.strings.intern("__fn");
                            access->type = expected;
                            e->args[i] = access;
                        }
                    } else {
                        e->args[i] = implicitWiden(e->args[i], expected);
                        if (e->args[i]->type != expected) {
                            ctx_.diag.error(call->args[i]->loc,
                                std::string("argument type mismatch: expected ") +
                                ctx_.types.name(expected) + ", got " +
                                ctx_.types.name(e->args[i]->type));
                        }
                    }
                }
            }
            e->type = ti.fn.return_type;
            return e;
        }
    }

    // Inside a lambda: if the callee is a captured Fn-typed variable, register
    // the capture and treat it as an indirect call (function pointer variable).
    if (in_lambda_) {
        auto outer_it = outer_locals_.find(call->callee);
        if (outer_it != outer_locals_.end()) {
            TypeId outer_type = outer_it->second;
            if (outer_type < ctx_.types.size() && ctx_.types.get(outer_type).kind == TypeKind::Fn) {
                auto interned = ctx_.strings.intern(call->callee);
                // Record capture
                bool already = false;
                for (auto& cap : current_captures_) {
                    if (cap.name == interned) { already = true; break; }
                }
                if (!already) {
                    current_captures_.push_back({interned, outer_type,
                        outer_mutables_.count(call->callee) > 0});
                }
                local_vars_[interned] = outer_type;
                // Rebuild as indirect call via the captured variable
                auto& ti = ctx_.types.get(outer_type);
                auto* e = ctx_.arena.make<HIRCallIndirectExpr>();
                e->kind = HIRExpr::Kind::CallIndirect;
                e->loc = expr->loc;
                e->is_tail_call = false;
                auto* callee_expr = ctx_.arena.make<HIRIdentExpr>();
                callee_expr->kind = HIRExpr::Kind::Ident;
                callee_expr->loc = expr->loc;
                callee_expr->name = interned;
                callee_expr->type = outer_type;
                e->callee = callee_expr;
                e->arg_count = call->arg_count;
                e->args = ctx_.arena.makeArray<HIRExpr*>(call->arg_count);
                for (uint32_t i = 0; i < call->arg_count; ++i) {
                    TypeId expected = (i < ti.fn.param_count) ? ti.fn.params[i] : INVALID_TYPE;
                    e->args[i] = buildExpr(call->args[i], expected);
                }
                e->type = ti.fn.return_type;
                return e;
            }
        }
    }

    // Direct call
    auto* e = ctx_.arena.make<HIRCallExpr>();
    e->kind = HIRExpr::Kind::Call;
    e->loc = expr->loc;
    e->callee = ctx_.strings.intern(call->callee);
    e->is_tail_call = false;

    // Set cross-module callee_module if this function was imported
    auto mod_it = fn_module_map_.find(call->callee);
    if (mod_it != fn_module_map_.end()) {
        e->callee_module = mod_it->second;
    }

    auto it = fn_table_.find(call->callee);
    if (it == fn_table_.end()) {
        ctx_.diag.error(expr->loc, std::string("undeclared function '") +
                    std::string(call->callee) + "'");
        e->type = TypeTable::Error;
        e->args = nullptr;
        e->arg_count = 0;
        return e;
    }

    const FnSig& sig = it->second;
    if (call->arg_count != sig.param_types.size()) {
        ctx_.diag.error(expr->loc, std::string("function '") + std::string(call->callee) +
                    "' expects " + std::to_string(sig.param_types.size()) +
                    " arguments, got " + std::to_string(call->arg_count));
        e->type = TypeTable::Error;
        e->args = nullptr;
        e->arg_count = 0;
        return e;
    }

    e->arg_count = call->arg_count;
    e->args = ctx_.arena.makeArray<HIRExpr*>(call->arg_count);

    // Check if this is a generic function call (any param contains TypeVar)
    bool is_generic_call = false;
    for (size_t i = 0; i < sig.param_types.size(); ++i) {
        if (containsTypeVar(ctx_.types, sig.param_types[i])) {
            is_generic_call = true;
            break;
        }
    }
    if (!is_generic_call && containsTypeVar(ctx_.types, sig.return_type)) {
        is_generic_call = true;
    }

    // Build type substitution map for generic calls
    std::unordered_map<TypeId, TypeId> type_subst;

    for (uint32_t i = 0; i < call->arg_count; ++i) {
        TypeId expected = sig.param_types[i];
        // For generic calls, don't pass types containing TypeVars as context
        std::optional<TypeId> ctx_t;
        if (!is_generic_call || !containsTypeVar(ctx_.types, expected)) {
            ctx_t = expected;
        }
        e->args[i] = buildExpr(call->args[i], ctx_t);

        if (is_generic_call && ctx_.types.get(expected).kind == TypeKind::TypeVar) {
            // Infer: TypeVar → actual arg type
            type_subst[expected] = e->args[i]->type;
        } else if (is_generic_call && containsTypeVar(ctx_.types, expected)) {
            // Deep match: parametric type containing TypeVars (e.g. Option<T>)
            if (!deepTypeMatch(ctx_.types, expected, e->args[i]->type, type_subst)) {
                ctx_.diag.error(call->args[i]->loc,
                    std::string("argument type mismatch: expected ") +
                    ctx_.types.name(expected) + ", got " +
                    ctx_.types.name(e->args[i]->type));
            }
        } else if (e->args[i]->type != TypeTable::Error && e->args[i]->type != expected) {
            if (typesMatchClosure(e->args[i]->type, expected, ctx_.types, closure_struct_types_)) {
                // Closure struct passed where Fn expected: extract __fn field
                if (closure_struct_types_.count(e->args[i]->type) &&
                    expected < ctx_.types.size() && ctx_.types.get(expected).kind == TypeKind::Fn) {
                    auto* access = ctx_.arena.make<HIRFieldAccessExpr>();
                    access->kind = HIRExpr::Kind::FieldAccess;
                    access->loc = call->args[i]->loc;
                    access->object = e->args[i];
                    access->field_name = ctx_.strings.intern("__fn");
                    access->type = expected;
                    e->args[i] = access;
                }
            } else {
                e->args[i] = implicitWiden(e->args[i], expected);
                if (e->args[i]->type != expected) {
                    ctx_.diag.error(call->args[i]->loc,
                        std::string("argument type mismatch: expected ") +
                        ctx_.types.name(expected) + ", got " +
                        ctx_.types.name(e->args[i]->type));
                }
            }
        }
    }

    // For generic calls, substitute TypeVars in return type
    TypeId ret_type = sig.return_type;
    if (is_generic_call) {
        ret_type = substituteTypeVars(ctx_.types, ret_type, type_subst);
    }

    e->type = ret_type;
    return e;
}

HIRExpr* HIRBuilder::buildIf(const Expr* expr, std::optional<TypeId> ctx_type) {
    auto* ifE = static_cast<const IfExpr*>(expr);
    auto* e = ctx_.arena.make<HIRIfExpr>();
    e->kind = HIRExpr::Kind::If;
    e->loc = expr->loc;

    e->condition = buildExpr(ifE->condition);
    if (e->condition->type != TypeTable::Error && e->condition->type != TypeTable::Bool) {
        ctx_.diag.error(ifE->condition->loc, "if condition must be bool");
    }

    e->then_branch = buildExpr(ifE->then_branch, ctx_type);

    if (ifE->else_branch) {
        e->else_branch = buildExpr(ifE->else_branch, ctx_type);
        TypeId then_t = e->then_branch->type;
        TypeId else_t = e->else_branch->type;
        if (then_t != TypeTable::Error && else_t != TypeTable::Error && !typesMatch(then_t, else_t)) {
            ctx_.diag.error(expr->loc, std::string("if branches have different types: ") +
                        ctx_.types.name(then_t) + " vs " + ctx_.types.name(else_t));
            e->type = TypeTable::Error;
        } else {
            e->type = mergeTypes(then_t, else_t);
        }
    } else {
        e->else_branch = nullptr;
        e->type = e->then_branch->type;
    }
    return e;
}

HIRExpr* HIRBuilder::buildBlock(const Expr* expr, std::optional<TypeId> ctx_type) {
    auto* block = static_cast<const BlockExpr*>(expr);
    auto* e = ctx_.arena.make<HIRBlockExpr>();
    e->kind = HIRExpr::Kind::Block;
    e->loc = expr->loc;

    e->stmt_count = block->stmt_count;
    e->stmts = ctx_.arena.makeArray<HIRStmt*>(block->stmt_count);
    for (uint32_t i = 0; i < block->stmt_count; ++i) {
        e->stmts[i] = buildStmt(block->stmts[i]);
    }

    if (block->result) {
        e->result = buildExpr(block->result, ctx_type);
        e->type = e->result->type;
    } else {
        e->result = nullptr;
        e->type = TypeTable::Unit;
    }
    return e;
}

HIRExpr* HIRBuilder::buildReturn(const Expr* expr) {
    auto* ret = static_cast<const ReturnExpr*>(expr);
    auto* e = ctx_.arena.make<HIRReturnExpr>();
    e->kind = HIRExpr::Kind::Return;
    e->loc = expr->loc;

    if (ret->value) {
        e->value = buildExpr(ret->value, current_return_type_);
        if (e->value->type != TypeTable::Error &&
            !typesMatchClosure(e->value->type, current_return_type_, ctx_.types, closure_struct_types_)) {
            e->value = implicitWiden(e->value, current_return_type_);
            if (e->value->type != current_return_type_) {
                ctx_.diag.error(expr->loc, std::string("return type mismatch: expected ") +
                            ctx_.types.name(current_return_type_) + ", got " +
                            ctx_.types.name(e->value->type));
            }
        }
        e->type = e->value->type;
    } else {
        e->value = nullptr;
        e->type = TypeTable::Unit;
    }
    return e;
}

HIRExpr* HIRBuilder::buildStructLit(const Expr* expr) {
    auto* sl = static_cast<const StructLitExpr*>(expr);
    auto* e = ctx_.arena.make<HIRStructLitExpr>();
    e->kind = HIRExpr::Kind::StructLit;
    e->loc = expr->loc;
    e->struct_name = ctx_.strings.intern(sl->struct_name);

    auto type_it = named_types_.find(sl->struct_name);

    // Slice<T> struct literal: infer element type from the 'data' field
    if ((type_it == named_types_.end() || type_it->second == INVALID_TYPE) &&
        sl->struct_name == "Slice") {
        // Find the 'data' field and build it to infer the element type
        for (uint32_t i = 0; i < sl->field_count; ++i) {
            if (sl->fields[i].name == "data") {
                auto* data_expr = buildExpr(sl->fields[i].value);
                if (data_expr->type != TypeTable::Error) {
                    const auto& dti = ctx_.types.get(data_expr->type);
                    if (dti.kind == TypeKind::Ptr || dti.kind == TypeKind::PtrMut) {
                        TypeId elem_type = dti.ptr.pointee;
                        std::string slice_name = "Slice_" + std::string(ctx_.types.name(elem_type));
                        auto interned = ctx_.strings.intern(slice_name);
                        auto sit = named_types_.find(interned);
                        if (sit == named_types_.end()) {
                            FieldInfo fields[] = {
                                {ctx_.strings.intern("data"), ctx_.types.makePtr(elem_type, false), false, 0},
                                {ctx_.strings.intern("len"), TypeTable::U64, false, 8},
                            };
                            TypeId tid = ctx_.types.makeStruct(interned, fields);
                            named_types_[interned] = tid;
                        }
                        type_it = named_types_.find(interned);
                        e->struct_name = interned;
                    }
                }
                break;
            }
        }
    }

    if (type_it == named_types_.end() || type_it->second == INVALID_TYPE) {
        ctx_.diag.error(expr->loc, std::string("unknown struct '") +
                    std::string(sl->struct_name) + "'");
        e->type = TypeTable::Error;
        e->fields = nullptr;
        e->field_count = 0;
        return e;
    }

    TypeId struct_tid = type_it->second;
    const auto& type_info = ctx_.types.get(struct_tid);
    if (type_info.kind != TypeKind::Struct) {
        e->type = TypeTable::Error;
        e->fields = nullptr;
        e->field_count = 0;
        return e;
    }

    // Check field count
    if (sl->field_count != type_info.struct_.field_count) {
        ctx_.diag.error(expr->loc, std::string("struct '") + std::string(sl->struct_name) +
                    "' expects " + std::to_string(type_info.struct_.field_count) +
                    " fields, got " + std::to_string(sl->field_count));
        e->type = TypeTable::Error;
        e->fields = nullptr;
        e->field_count = 0;
        return e;
    }

    e->field_count = sl->field_count;
    e->fields = ctx_.arena.makeArray<HIRFieldInit>(sl->field_count);
    bool has_error = false;

    for (uint32_t i = 0; i < sl->field_count; ++i) {
        bool found = false;
        for (uint32_t j = 0; j < type_info.struct_.field_count; ++j) {
            const auto& fd = type_info.struct_.fields[j];
            if (fd.name == sl->fields[i].name) {
                found = true;
                auto* val = buildExpr(sl->fields[i].value, fd.type);
                e->fields[i] = {ctx_.strings.intern(sl->fields[i].name), val, sl->fields[i].loc};
                if (val->type != TypeTable::Error && val->type != fd.type) {
                    ctx_.diag.error(sl->fields[i].loc,
                        std::string("field '") + std::string(fd.name) +
                        "' expects " + ctx_.types.name(fd.type) +
                        ", got " + ctx_.types.name(val->type));
                    has_error = true;
                }
                break;
            }
        }
        if (!found) {
            ctx_.diag.error(sl->fields[i].loc,
                std::string("struct '") + std::string(sl->struct_name) +
                "' has no field named '" + std::string(sl->fields[i].name) + "'");
            e->fields[i] = {ctx_.strings.intern(sl->fields[i].name),
                            buildExpr(sl->fields[i].value), sl->fields[i].loc};
            has_error = true;
        }
    }

    e->type = has_error ? TypeTable::Error : struct_tid;
    return e;
}

HIRExpr* HIRBuilder::buildFieldAccess(const Expr* expr) {
    auto* fa = static_cast<const FieldAccessExpr*>(expr);
    auto* e = ctx_.arena.make<HIRFieldAccessExpr>();
    e->kind = HIRExpr::Kind::FieldAccess;
    e->loc = expr->loc;
    e->field_name = ctx_.strings.intern(fa->field_name);
    e->object = buildExpr(fa->object);

    TypeId obj_type = e->object->type;
    if (obj_type == TypeTable::Error) {
        e->type = TypeTable::Error;
        return e;
    }

    const auto& obj_info = ctx_.types.get(obj_type);

    // String field access
    if (obj_info.kind == TypeKind::Struct && obj_info.struct_.name == "String") {
        if (fa->field_name == "len") {
            e->type = TypeTable::U64;
        } else if (fa->field_name == "data") {
            e->type = ctx_.types.makePtr(TypeTable::U8, false);
        } else {
            ctx_.diag.error(expr->loc, std::string("String has no field named '") +
                        std::string(fa->field_name) + "'");
            e->type = TypeTable::Error;
        }
        return e;
    }

    if (obj_info.kind != TypeKind::Struct) {
        ctx_.diag.error(expr->loc, std::string("field access requires struct type, got ") +
                    ctx_.types.name(obj_type));
        e->type = TypeTable::Error;
        return e;
    }

    // Look up field
    for (uint32_t i = 0; i < obj_info.struct_.field_count; ++i) {
        if (obj_info.struct_.fields[i].name == fa->field_name) {
            e->type = obj_info.struct_.fields[i].type;
            return e;
        }
    }

    ctx_.diag.error(expr->loc, std::string("struct '") + std::string(obj_info.struct_.name) +
                "' has no field named '" + std::string(fa->field_name) + "'");
    e->type = TypeTable::Error;
    return e;
}

HIRExpr* HIRBuilder::buildEnumAccess(const Expr* expr) {
    auto* ea = static_cast<const EnumAccessExpr*>(expr);
    auto* e = ctx_.arena.make<HIREnumAccessExpr>();
    e->kind = HIRExpr::Kind::EnumAccess;
    e->loc = expr->loc;
    e->enum_name = ctx_.strings.intern(ea->enum_name);
    e->variant_name = ctx_.strings.intern(ea->variant_name);

    auto type_it = named_types_.find(ea->enum_name);
    if (type_it == named_types_.end()) {
        ctx_.diag.error(expr->loc, std::string("unknown enum '") +
                    std::string(ea->enum_name) + "'");
        e->type = TypeTable::Error;
        return e;
    }

    TypeId enum_tid = type_it->second;
    const auto& info = ctx_.types.get(enum_tid);
    if (info.kind != TypeKind::Enum) {
        e->type = TypeTable::Error;
        return e;
    }

    bool found = false;
    for (uint32_t i = 0; i < info.enum_.variant_count; ++i) {
        if (info.enum_.names[i] == ea->variant_name) {
            found = true;
            break;
        }
    }

    if (!found) {
        ctx_.diag.error(expr->loc, std::string("enum '") + std::string(ea->enum_name) +
                    "' has no variant '" + std::string(ea->variant_name) + "'");
        e->type = TypeTable::Error;
        return e;
    }

    e->type = enum_tid;
    return e;
}

HIRExpr* HIRBuilder::buildUnionVariant(const Expr* expr) {
    auto* uv = static_cast<const UnionVariantExpr*>(expr);
    auto* e = ctx_.arena.make<HIRUnionVariantExpr>();
    e->kind = HIRExpr::Kind::UnionVariant;
    e->loc = expr->loc;
    e->union_name = ctx_.strings.intern(uv->union_name);
    e->variant_name = ctx_.strings.intern(uv->variant_name);

    auto type_it = named_types_.find(uv->union_name);
    if (type_it == named_types_.end()) {
        ctx_.diag.error(expr->loc, std::string("unknown union '") +
                    std::string(uv->union_name) + "'");
        e->type = TypeTable::Error;
        e->payload = nullptr;
        return e;
    }

    TypeId union_tid = type_it->second;
    const auto& info = ctx_.types.get(union_tid);
    if (info.kind != TypeKind::Union) {
        e->type = TypeTable::Error;
        e->payload = nullptr;
        return e;
    }

    const VariantInfo* vinfo = nullptr;
    for (uint32_t i = 0; i < info.union_.variant_count; ++i) {
        if (info.union_.variants[i].name == uv->variant_name) {
            vinfo = &info.union_.variants[i];
            break;
        }
    }

    if (!vinfo) {
        ctx_.diag.error(expr->loc, std::string("union '") + std::string(uv->union_name) +
                    "' has no variant '" + std::string(uv->variant_name) + "'");
        e->type = TypeTable::Error;
        e->payload = nullptr;
        return e;
    }

    bool has_payload = vinfo->payload_type != INVALID_TYPE;
    if (!has_payload && uv->payload) {
        ctx_.diag.error(expr->loc, std::string("variant '") + std::string(uv->variant_name) +
                    "' takes no payload");
        e->type = TypeTable::Error;
        e->payload = nullptr;
        return e;
    }
    if (has_payload && !uv->payload) {
        ctx_.diag.error(expr->loc, std::string("variant '") + std::string(uv->variant_name) +
                    "' requires a payload of type " + ctx_.types.name(vinfo->payload_type));
        e->type = TypeTable::Error;
        e->payload = nullptr;
        return e;
    }

    if (uv->payload) {
        e->payload = buildExpr(uv->payload, vinfo->payload_type);
        if (e->payload->type != TypeTable::Error && e->payload->type != vinfo->payload_type) {
            ctx_.diag.error(uv->payload->loc,
                std::string("variant '") + std::string(uv->variant_name) +
                "' expects payload of type " + ctx_.types.name(vinfo->payload_type) +
                ", got " + ctx_.types.name(e->payload->type));
            e->type = TypeTable::Error;
            return e;
        }
    } else {
        e->payload = nullptr;
    }

    e->type = union_tid;
    return e;
}

// ============================================================================
// Cast building
// ============================================================================

HIRExpr* HIRBuilder::buildCast(const Expr* expr) {
    auto* cast_expr = static_cast<const CastExpr*>(expr);
    TypeId target_type = resolveType(cast_expr->target);
    if (target_type == TypeTable::Error) {
        return errorExpr(expr->loc);
    }

    HIRExpr* operand = buildExpr(cast_expr->operand);
    if (operand->type == TypeTable::Error) {
        return errorExpr(expr->loc);
    }

    TypeId src_type = operand->type;

    // Validate cast: int↔int, int↔ptr, ptr↔ptr, float↔float, float↔int
    bool src_int = isIntegerType(src_type);
    bool dst_int = isIntegerType(target_type);
    bool src_float = ctx_.types.isFloat(src_type);
    bool dst_float = ctx_.types.isFloat(target_type);
    bool src_ptr = false, dst_ptr = false;
    if (src_type < ctx_.types.size()) {
        auto k = ctx_.types.get(src_type).kind;
        src_ptr = (k == TypeKind::Ptr || k == TypeKind::PtrMut);
    }
    if (target_type < ctx_.types.size()) {
        auto k = ctx_.types.get(target_type).kind;
        dst_ptr = (k == TypeKind::Ptr || k == TypeKind::PtrMut);
    }

    // Check if target is dyn Trait — construct fat pointer
    bool dst_dyn = false;
    if (target_type < ctx_.types.size()) {
        dst_dyn = (ctx_.types.get(target_type).kind == TypeKind::DynTrait);
    }

    if (dst_dyn) {
        const auto& dyn_info = ctx_.types.get(target_type).dyn_trait;
        std::string_view trait_name = dyn_info.trait_name;

        // Look up the concrete type's impl for this trait
        std::string_view type_name = actualTypeName(src_type, ctx_.types);
        auto impl_it = impl_table_.find(type_name);
        if (impl_it == impl_table_.end()) {
            ctx_.diag.error(expr->loc, std::string("type '") + std::string(type_name) +
                "' does not implement trait '" + std::string(trait_name) + "'");
            return errorExpr(expr->loc);
        }

        // Build vtable label and function pointer list
        std::string vtbl_name = "__vtbl_" + std::string(type_name) + "_" + std::string(trait_name);
        auto vtbl_label = ctx_.strings.intern(vtbl_name);

        // Check if vtable already registered
        bool found = false;
        for (auto& vt : vtable_globals_) {
            if (vt.label == vtbl_label) { found = true; break; }
        }
        if (!found) {
            VTableInfo vt;
            vt.label = vtbl_label;
            vt.trait_name = trait_name;
            vt.type_name = type_name;
            // Fill fn_labels in trait method order
            auto trait_it = trait_table_.find(trait_name);
            if (trait_it != trait_table_.end()) {
                for (auto& mname : trait_it->second.method_names) {
                    auto mit = impl_it->second.methods.find(mname);
                    if (mit != impl_it->second.methods.end()) {
                        vt.fn_labels.push_back(mit->second);
                    } else {
                        ctx_.diag.error(expr->loc, std::string("missing method '") +
                            std::string(mname) + "' in impl " + std::string(trait_name) +
                            " for " + std::string(type_name));
                        return errorExpr(expr->loc);
                    }
                }
            }
            vtable_globals_.push_back(std::move(vt));
        }

        // Construct fat pointer: take addr of operand, addr of vtable
        // The result is a DynTrait-typed value (16 bytes: data_ptr + vtable_ptr)
        // We'll use an AddrOf for the operand to get a Ptr<u8> data pointer
        auto* addr_expr = ctx_.arena.make<HIRAddrOfExpr>();
        addr_expr->kind = HIRExpr::Kind::AddrOf;
        addr_expr->loc = expr->loc;
        addr_expr->operand = operand;
        addr_expr->is_mutable = false;
        addr_expr->type = ctx_.types.makePtr(src_type, false);

        // Create a cast from Ptr<ConcreteType> to Ptr<u8>
        auto* data_cast = ctx_.arena.make<HIRCastExpr>();
        data_cast->kind = HIRExpr::Kind::Cast;
        data_cast->loc = expr->loc;
        data_cast->type = ctx_.types.makePtr(TypeTable::U8, false);
        data_cast->operand = addr_expr;
        data_cast->target_type = data_cast->type;

        // The vtable reference is a global label (LeaGlobal in LIR)
        auto* vtbl_ref = ctx_.arena.make<HIRIdentExpr>();
        vtbl_ref->kind = HIRExpr::Kind::Ident;
        vtbl_ref->loc = expr->loc;
        vtbl_ref->name = vtbl_label;
        vtbl_ref->type = ctx_.types.makePtr(TypeTable::U64, false);

        // Build a struct literal for the fat pointer: { __data, __vtable }
        auto* slit = ctx_.arena.make<HIRStructLitExpr>();
        slit->kind = HIRExpr::Kind::StructLit;
        slit->loc = expr->loc;
        slit->type = target_type;
        slit->struct_name = ctx_.strings.intern("__dyn");
        slit->field_count = 2;
        slit->fields = ctx_.arena.makeArray<HIRFieldInit>(2);
        slit->fields[0].name = ctx_.strings.intern("__data");
        slit->fields[0].value = data_cast;
        slit->fields[1].name = ctx_.strings.intern("__vtable");
        slit->fields[1].value = vtbl_ref;
        return slit;
    }

    // Check for enum types
    bool src_enum = false, dst_enum = false;
    if (src_type < ctx_.types.size())
        src_enum = (ctx_.types.get(src_type).kind == TypeKind::Enum);
    if (target_type < ctx_.types.size())
        dst_enum = (ctx_.types.get(target_type).kind == TypeKind::Enum);

    bool valid = (src_int && dst_int) ||
                 (src_int && dst_ptr) ||
                 (src_ptr && dst_int) ||
                 (src_ptr && dst_ptr) ||
                 (src_float && dst_float) ||
                 (src_float && dst_int) ||
                 (src_int && dst_float) ||
                 (src_enum && dst_int) ||   // enum → int
                 (src_int && dst_enum) ||   // int → enum
                 (src_enum && dst_enum) ||  // enum → enum
                 (src_type == target_type);

    if (!valid) {
        ctx_.diag.error(expr->loc, std::string("cannot cast ") +
            ctx_.types.name(src_type) + " to " + ctx_.types.name(target_type));
        return errorExpr(expr->loc);
    }

    // If same type, just return the operand
    if (src_type == target_type) {
        return operand;
    }

    auto* e = ctx_.arena.make<HIRCastExpr>();
    e->kind = HIRExpr::Kind::Cast;
    e->loc = expr->loc;
    e->type = target_type;
    e->operand = operand;
    e->target_type = target_type;
    return e;
}

// ============================================================================
// Loop building
// ============================================================================

HIRExpr* HIRBuilder::buildLoop(const Expr* expr, std::optional<TypeId> ctx_type) {
    auto* loop = static_cast<const LoopExpr*>(expr);
    auto* e = ctx_.arena.make<HIRLoopExpr>();
    e->kind = HIRExpr::Kind::Loop;
    e->loc = expr->loc;

    // Build bindings
    e->binding_count = loop->binding_count;
    e->bindings = ctx_.arena.makeArray<HIRLoopBinding>(loop->binding_count);

    for (uint32_t i = 0; i < loop->binding_count; ++i) {
        auto& src = loop->bindings[i];
        auto& dst = e->bindings[i];
        dst.name = ctx_.strings.intern(src.name);
        dst.loc = src.init->loc;
        dst.init = buildExpr(src.init);
        dst.type = dst.init->type;
        // Register binding in scope
        local_vars_[dst.name] = dst.type;
    }

    // Build body as block expression
    auto* body = ctx_.arena.make<HIRBlockExpr>();
    body->kind = HIRExpr::Kind::Block;
    body->loc = expr->loc;

    std::vector<HIRStmt*> stmts_vec;
    for (uint32_t i = 0; i < loop->stmt_count; ++i) {
        stmts_vec.push_back(buildStmt(loop->stmts[i]));
    }
    body->stmt_count = static_cast<uint32_t>(stmts_vec.size());
    body->stmts = ctx_.arena.makeArray<HIRStmt*>(body->stmt_count);
    for (uint32_t i = 0; i < body->stmt_count; ++i) {
        body->stmts[i] = stmts_vec[i];
    }

    if (loop->result) {
        body->result = buildExpr(loop->result);
        body->type = body->result->type;
    } else {
        body->result = nullptr;
        body->type = TypeTable::Unit;
    }

    e->body = body;
    e->label = loop->label;
    // Loop type = inferred from break values or ctx_type
    if (ctx_type.has_value()) {
        e->type = *ctx_type;
    } else {
        e->type = TypeTable::Unit;
    }

    return e;
}

// ============================================================================
// For-range desugaring
// ============================================================================

HIRExpr* HIRBuilder::buildForRange(const Expr* expr) {
    auto* fr = static_cast<const ForRangeExpr*>(expr);

    // Desugar: for i in start..end { body }
    // → loop(i = start) { if i >= end { break }; body; continue(i + 1) }

    auto* loop = ctx_.arena.make<HIRLoopExpr>();
    loop->kind = HIRExpr::Kind::Loop;
    loop->loc = expr->loc;
    loop->type = TypeTable::Unit;
    loop->label = fr->label;

    // Build start expression to infer iterator type
    HIRExpr* start_expr = buildExpr(fr->start);
    TypeId iter_type = start_expr->type;

    // Single binding: i = start
    loop->binding_count = 1;
    loop->bindings = ctx_.arena.makeArray<HIRLoopBinding>(1);
    loop->bindings[0].name = ctx_.strings.intern(fr->var_name);
    loop->bindings[0].loc = fr->start->loc;
    loop->bindings[0].init = start_expr;
    loop->bindings[0].type = iter_type;

    // Register binding in scope
    local_vars_[loop->bindings[0].name] = iter_type;

    // Build end expression
    HIRExpr* end_expr = buildExpr(fr->end, iter_type);

    // Build body block containing:
    //   1. if i >= end { break }
    //   2. user body stmts
    //   3. continue(i + 1)

    auto* body = ctx_.arena.make<HIRBlockExpr>();
    body->kind = HIRExpr::Kind::Block;
    body->loc = expr->loc;
    body->type = TypeTable::Unit;
    body->result = nullptr;

    std::vector<HIRStmt*> stmts_vec;

    // 1. Synthesize: if i >= end { break }
    {
        // Reference to i
        auto* i_ref = ctx_.arena.make<HIRIdentExpr>();
        i_ref->kind = HIRExpr::Kind::Ident;
        i_ref->loc = expr->loc;
        i_ref->name = loop->bindings[0].name;
        i_ref->type = iter_type;

        // i >= end
        auto* cmp = ctx_.arena.make<HIRBinOpExpr>();
        cmp->kind = HIRExpr::Kind::BinOp;
        cmp->loc = expr->loc;
        cmp->op = HIRBinOp::GtEq;
        cmp->lhs = i_ref;
        cmp->rhs = end_expr;
        cmp->type = TypeTable::Bool;

        // break (no value)
        auto* brk = ctx_.arena.make<HIRBreakExpr>();
        brk->kind = HIRExpr::Kind::Break;
        brk->loc = expr->loc;
        brk->value = nullptr;
        brk->type = TypeTable::Unit;

        // if cmp { break }
        auto* if_expr = ctx_.arena.make<HIRIfExpr>();
        if_expr->kind = HIRExpr::Kind::If;
        if_expr->loc = expr->loc;
        if_expr->condition = cmp;
        if_expr->then_branch = brk;
        if_expr->else_branch = nullptr;
        if_expr->type = TypeTable::Unit;

        auto* guard_stmt = ctx_.arena.make<HIRExprStmt>();
        guard_stmt->kind = HIRStmt::Kind::ExprStmt;
        guard_stmt->loc = expr->loc;
        guard_stmt->expr = if_expr;
        stmts_vec.push_back(guard_stmt);
    }

    // 2. Build user body statements
    for (uint32_t i = 0; i < fr->stmt_count; ++i) {
        stmts_vec.push_back(buildStmt(fr->stmts[i]));
    }

    // 3. Synthesize: continue(i + 1)
    {
        // Reference to i
        auto* i_ref = ctx_.arena.make<HIRIdentExpr>();
        i_ref->kind = HIRExpr::Kind::Ident;
        i_ref->loc = expr->loc;
        i_ref->name = loop->bindings[0].name;
        i_ref->type = iter_type;

        // Constant 1
        auto* one = ctx_.arena.make<HIRIntLitExpr>();
        one->kind = HIRExpr::Kind::IntLit;
        one->loc = expr->loc;
        one->value = 1;
        one->type = iter_type;

        // i + 1
        auto* add = ctx_.arena.make<HIRBinOpExpr>();
        add->kind = HIRExpr::Kind::BinOp;
        add->loc = expr->loc;
        add->op = HIRBinOp::Add;
        add->lhs = i_ref;
        add->rhs = one;
        add->type = iter_type;

        // continue(i + 1)
        auto* cont = ctx_.arena.make<HIRContinueExpr>();
        cont->kind = HIRExpr::Kind::Continue;
        cont->loc = expr->loc;
        cont->arg_count = 1;
        cont->args = ctx_.arena.makeArray<HIRExpr*>(1);
        cont->args[0] = add;
        cont->type = TypeTable::Unit;

        auto* cont_stmt = ctx_.arena.make<HIRExprStmt>();
        cont_stmt->kind = HIRStmt::Kind::ExprStmt;
        cont_stmt->loc = expr->loc;
        cont_stmt->expr = cont;
        stmts_vec.push_back(cont_stmt);
    }

    body->stmt_count = static_cast<uint32_t>(stmts_vec.size());
    body->stmts = ctx_.arena.makeArray<HIRStmt*>(body->stmt_count);
    for (uint32_t i = 0; i < body->stmt_count; ++i) {
        body->stmts[i] = stmts_vec[i];
    }

    loop->body = body;
    return loop;
}

// ============================================================================
// For-each desugaring: for item in collection { body }
// → loop(__i = 0) { if __i >= len { break }; val item = collection[__i]; body; continue(__i + 1) }
// ============================================================================

HIRExpr* HIRBuilder::buildForEach(const Expr* expr) {
    auto* fe = static_cast<const ForEachExpr*>(expr);

    // Build collection expression to get its type
    HIRExpr* col_expr = buildExpr(fe->collection);
    TypeId col_type = col_expr->type;
    const auto& col_info = ctx_.types.get(col_type);

    // Determine element type and length
    TypeId elem_type = TypeTable::I64;
    int64_t array_len = 0;
    bool is_array = (col_info.kind == TypeKind::Array);
    bool is_slice = false;

    if (is_array) {
        elem_type = col_info.array.element;
        array_len = static_cast<int64_t>(col_info.array.count);
    } else if (col_info.kind == TypeKind::Struct && col_info.struct_.field_count == 2) {
        // Slice: { ptr: Ptr<T>, len: u64 }
        auto ptr_type = col_info.struct_.fields[0].type;
        auto& ptr_info = ctx_.types.get(ptr_type);
        if (ptr_info.kind == TypeKind::Ptr || ptr_info.kind == TypeKind::PtrMut) {
            elem_type = ptr_info.ptr.pointee;
            is_slice = true;
        }
    }

    if (!is_array && !is_slice) {
        ctx_.diag.error(expr->loc, "for-each requires an array or slice");
        auto* unit = ctx_.arena.make<HIRIntLitExpr>();
        unit->kind = HIRExpr::Kind::IntLit;
        unit->loc = expr->loc;
        unit->value = 0;
        unit->type = TypeTable::Unit;
        return unit;
    }

    auto* loop = ctx_.arena.make<HIRLoopExpr>();
    loop->kind = HIRExpr::Kind::Loop;
    loop->loc = expr->loc;
    loop->type = TypeTable::Unit;
    loop->label = fe->label;

    TypeId iter_type = TypeTable::I64;

    // Binding: __i = 0
    auto* zero = ctx_.arena.make<HIRIntLitExpr>();
    zero->kind = HIRExpr::Kind::IntLit;
    zero->loc = expr->loc;
    zero->value = 0;
    zero->type = iter_type;

    std::string_view idx_name = ctx_.strings.intern("__foreach_i");

    loop->binding_count = 1;
    loop->bindings = ctx_.arena.makeArray<HIRLoopBinding>(1);
    loop->bindings[0].name = idx_name;
    loop->bindings[0].loc = expr->loc;
    loop->bindings[0].init = zero;
    loop->bindings[0].type = iter_type;

    local_vars_[idx_name] = iter_type;

    // Build length expression
    HIRExpr* len_expr;
    if (is_array) {
        auto* len_lit = ctx_.arena.make<HIRIntLitExpr>();
        len_lit->kind = HIRExpr::Kind::IntLit;
        len_lit->loc = expr->loc;
        len_lit->value = array_len;
        len_lit->type = iter_type;
        len_expr = len_lit;
    } else {
        // Slice: load .len field (field index 1)
        auto* len_access = ctx_.arena.make<HIRFieldAccessExpr>();
        len_access->kind = HIRExpr::Kind::FieldAccess;
        len_access->loc = expr->loc;
        len_access->object = col_expr;
        len_access->field_name = ctx_.strings.intern("len");
        len_access->type = TypeTable::U64;

        // Cast u64 to i64 for comparison with loop counter
        auto* cast = ctx_.arena.make<HIRCastExpr>();
        cast->kind = HIRExpr::Kind::Cast;
        cast->loc = expr->loc;
        cast->operand = len_access;
        cast->target_type = iter_type;
        cast->type = iter_type;
        len_expr = cast;
    }

    // Build body
    auto* body = ctx_.arena.make<HIRBlockExpr>();
    body->kind = HIRExpr::Kind::Block;
    body->loc = expr->loc;
    body->type = TypeTable::Unit;
    body->result = nullptr;

    std::vector<HIRStmt*> stmts_vec;

    // 1. Guard: if __i >= len { break }
    {
        auto* i_ref = ctx_.arena.make<HIRIdentExpr>();
        i_ref->kind = HIRExpr::Kind::Ident;
        i_ref->loc = expr->loc;
        i_ref->name = idx_name;
        i_ref->type = iter_type;

        auto* cmp = ctx_.arena.make<HIRBinOpExpr>();
        cmp->kind = HIRExpr::Kind::BinOp;
        cmp->loc = expr->loc;
        cmp->op = HIRBinOp::GtEq;
        cmp->lhs = i_ref;
        cmp->rhs = len_expr;
        cmp->type = TypeTable::Bool;

        auto* brk = ctx_.arena.make<HIRBreakExpr>();
        brk->kind = HIRExpr::Kind::Break;
        brk->loc = expr->loc;
        brk->value = nullptr;
        brk->type = TypeTable::Unit;

        auto* if_expr = ctx_.arena.make<HIRIfExpr>();
        if_expr->kind = HIRExpr::Kind::If;
        if_expr->loc = expr->loc;
        if_expr->condition = cmp;
        if_expr->then_branch = brk;
        if_expr->else_branch = nullptr;
        if_expr->type = TypeTable::Unit;

        auto* guard_stmt = ctx_.arena.make<HIRExprStmt>();
        guard_stmt->kind = HIRStmt::Kind::ExprStmt;
        guard_stmt->loc = expr->loc;
        guard_stmt->expr = if_expr;
        stmts_vec.push_back(guard_stmt);
    }

    // 2. Bind element: val item = collection[__i]
    {
        auto* i_ref = ctx_.arena.make<HIRIdentExpr>();
        i_ref->kind = HIRExpr::Kind::Ident;
        i_ref->loc = expr->loc;
        i_ref->name = idx_name;
        i_ref->type = iter_type;

        auto* index_access = ctx_.arena.make<HIRIndexAccessExpr>();
        index_access->kind = HIRExpr::Kind::IndexAccess;
        index_access->loc = expr->loc;
        index_access->array = col_expr;
        index_access->index = i_ref;
        index_access->type = elem_type;

        auto* val_stmt = ctx_.arena.make<HIRValDeclStmt>();
        val_stmt->kind = HIRStmt::Kind::ValDecl;
        val_stmt->loc = expr->loc;
        val_stmt->name = ctx_.strings.intern(fe->var_name);
        val_stmt->type = elem_type;
        val_stmt->init = index_access;
        stmts_vec.push_back(val_stmt);

        local_vars_[val_stmt->name] = elem_type;
    }

    // 3. Build user body statements
    for (uint32_t i = 0; i < fe->stmt_count; ++i) {
        stmts_vec.push_back(buildStmt(fe->stmts[i]));
    }

    // 4. continue(__i + 1)
    {
        auto* i_ref = ctx_.arena.make<HIRIdentExpr>();
        i_ref->kind = HIRExpr::Kind::Ident;
        i_ref->loc = expr->loc;
        i_ref->name = idx_name;
        i_ref->type = iter_type;

        auto* one = ctx_.arena.make<HIRIntLitExpr>();
        one->kind = HIRExpr::Kind::IntLit;
        one->loc = expr->loc;
        one->value = 1;
        one->type = iter_type;

        auto* add = ctx_.arena.make<HIRBinOpExpr>();
        add->kind = HIRExpr::Kind::BinOp;
        add->loc = expr->loc;
        add->op = HIRBinOp::Add;
        add->lhs = i_ref;
        add->rhs = one;
        add->type = iter_type;

        auto* cont = ctx_.arena.make<HIRContinueExpr>();
        cont->kind = HIRExpr::Kind::Continue;
        cont->loc = expr->loc;
        cont->arg_count = 1;
        cont->args = ctx_.arena.makeArray<HIRExpr*>(1);
        cont->args[0] = add;
        cont->type = TypeTable::Unit;

        auto* cont_stmt = ctx_.arena.make<HIRExprStmt>();
        cont_stmt->kind = HIRStmt::Kind::ExprStmt;
        cont_stmt->loc = expr->loc;
        cont_stmt->expr = cont;
        stmts_vec.push_back(cont_stmt);
    }

    body->stmt_count = static_cast<uint32_t>(stmts_vec.size());
    body->stmts = ctx_.arena.makeArray<HIRStmt*>(body->stmt_count);
    for (uint32_t i = 0; i < body->stmt_count; ++i) {
        body->stmts[i] = stmts_vec[i];
    }

    loop->body = body;
    return loop;
}

// ============================================================================
// While loop desugaring
// ============================================================================

HIRExpr* HIRBuilder::buildWhileLoop(const Expr* expr) {
    auto* wl = static_cast<const WhileLoopExpr*>(expr);

    // Desugar: while cond { body }
    // → loop { if not cond { break }; body }

    auto* loop = ctx_.arena.make<HIRLoopExpr>();
    loop->kind = HIRExpr::Kind::Loop;
    loop->loc = expr->loc;
    loop->type = TypeTable::Unit;
    loop->label = wl->label;
    loop->binding_count = 0;
    loop->bindings = nullptr;

    auto* body = ctx_.arena.make<HIRBlockExpr>();
    body->kind = HIRExpr::Kind::Block;
    body->loc = expr->loc;
    body->type = TypeTable::Unit;
    body->result = nullptr;

    std::vector<HIRStmt*> stmts_vec;

    // 1. Synthesize: if not cond { break }
    {
        HIRExpr* cond = buildExpr(wl->condition);

        // not cond
        auto* neg = ctx_.arena.make<HIRUnaryOpExpr>();
        neg->kind = HIRExpr::Kind::UnaryOp;
        neg->loc = expr->loc;
        neg->op = HIRUnaryOp::Not;
        neg->operand = cond;
        neg->type = TypeTable::Bool;

        // break
        auto* brk = ctx_.arena.make<HIRBreakExpr>();
        brk->kind = HIRExpr::Kind::Break;
        brk->loc = expr->loc;
        brk->value = nullptr;
        brk->type = TypeTable::Unit;

        // if not cond { break }
        auto* if_expr = ctx_.arena.make<HIRIfExpr>();
        if_expr->kind = HIRExpr::Kind::If;
        if_expr->loc = expr->loc;
        if_expr->condition = neg;
        if_expr->then_branch = brk;
        if_expr->else_branch = nullptr;
        if_expr->type = TypeTable::Unit;

        auto* guard = ctx_.arena.make<HIRExprStmt>();
        guard->kind = HIRStmt::Kind::ExprStmt;
        guard->loc = expr->loc;
        guard->expr = if_expr;
        stmts_vec.push_back(guard);
    }

    // 2. Build user body statements
    for (uint32_t i = 0; i < wl->stmt_count; ++i) {
        stmts_vec.push_back(buildStmt(wl->stmts[i]));
    }

    // 3. Synthesize: continue() — jump back to loop header
    {
        auto* cont = ctx_.arena.make<HIRContinueExpr>();
        cont->kind = HIRExpr::Kind::Continue;
        cont->loc = expr->loc;
        cont->arg_count = 0;
        cont->args = nullptr;
        cont->type = TypeTable::Unit;

        auto* cont_stmt = ctx_.arena.make<HIRExprStmt>();
        cont_stmt->kind = HIRStmt::Kind::ExprStmt;
        cont_stmt->loc = expr->loc;
        cont_stmt->expr = cont;
        stmts_vec.push_back(cont_stmt);
    }

    body->stmt_count = static_cast<uint32_t>(stmts_vec.size());
    body->stmts = ctx_.arena.makeArray<HIRStmt*>(body->stmt_count);
    for (uint32_t i = 0; i < body->stmt_count; ++i) {
        body->stmts[i] = stmts_vec[i];
    }

    loop->body = body;
    return loop;
}

// ============================================================================
// Array building
// ============================================================================

HIRExpr* HIRBuilder::buildArrayLit(const Expr* expr) {
    auto* arr = static_cast<const ArrayLitExpr*>(expr);
    auto* e = ctx_.arena.make<HIRArrayLitExpr>();
    e->kind = HIRExpr::Kind::ArrayLit;
    e->loc = expr->loc;
    e->element_count = arr->count;
    e->elements = ctx_.arena.makeArray<HIRExpr*>(arr->count);

    // Build each element
    TypeId elem_type = TypeTable::Error;
    for (uint32_t i = 0; i < arr->count; ++i) {
        e->elements[i] = buildExpr(arr->elements[i]);
        if (i == 0) {
            elem_type = e->elements[i]->type;
        }
    }

    // Create array type [ElemType; N]
    if (elem_type != TypeTable::Error) {
        e->type = ctx_.types.makeArrayType(elem_type, arr->count);
    } else {
        e->type = TypeTable::Error;
    }
    return e;
}

HIRExpr* HIRBuilder::buildIndexAccess(const Expr* expr) {
    auto* ia = static_cast<const IndexAccessExpr*>(expr);
    auto* e = ctx_.arena.make<HIRIndexAccessExpr>();
    e->kind = HIRExpr::Kind::IndexAccess;
    e->loc = expr->loc;
    e->array = buildExpr(ia->array);
    e->index = buildExpr(ia->index);

    // Determine element type from array/slice/pointer type
    TypeId arr_type = e->array->type;
    if (arr_type != TypeTable::Error && arr_type < ctx_.types.size()) {
        const auto& ti = ctx_.types.get(arr_type);
        if (ti.kind == TypeKind::Array) {
            e->type = ti.array.element;
            // Constant bounds check: arr[N] where N >= array size
            if (e->index->kind == HIRExpr::Kind::IntLit) {
                auto* idx = static_cast<const HIRIntLitExpr*>(e->index);
                if (idx->value < 0 || static_cast<uint64_t>(idx->value) >= ti.array.count) {
                    ctx_.diag.error(expr->loc,
                        "index " + std::to_string(idx->value) + " out of bounds for array of size " +
                        std::to_string(ti.array.count));
                    e->type = TypeTable::Error;
                }
            }
        } else if (ti.kind == TypeKind::Struct &&
                   ti.struct_.name.substr(0, 6) == "Slice_" &&
                   ti.struct_.field_count >= 2) {
            // Slice<T> indexing: desugar s[i] to array index on s.data pointer
            // Replace array operand with s.data (field access)
            auto* data_access = ctx_.arena.make<HIRFieldAccessExpr>();
            data_access->kind = HIRExpr::Kind::FieldAccess;
            data_access->loc = expr->loc;
            data_access->object = e->array;
            data_access->field_name = ctx_.strings.intern("data");
            data_access->type = ti.struct_.fields[0].type; // Ptr<T>

            // Extract element type from pointer
            const auto& ptr_ti = ctx_.types.get(data_access->type);
            if (ptr_ti.kind == TypeKind::Ptr || ptr_ti.kind == TypeKind::PtrMut) {
                e->type = ptr_ti.ptr.pointee;
            } else {
                e->type = TypeTable::Error;
            }

            // Replace array with the .data pointer for LIR lowering
            e->array = data_access;
        } else if (ti.kind == TypeKind::Ptr || ti.kind == TypeKind::PtrMut) {
            // Pointer indexing: p[i] = *(p + i * sizeof(pointee))
            e->type = ti.ptr.pointee;
        } else {
            ctx_.diag.error(expr->loc, "index access on non-array type");
            e->type = TypeTable::Error;
        }
    } else {
        e->type = TypeTable::Error;
    }
    return e;
}

HIRExpr* HIRBuilder::buildSliceExpr(const Expr* expr) {
    auto* se = static_cast<const SliceExprNode*>(expr);
    auto* arr_expr = buildExpr(se->array);
    TypeId arr_type = arr_expr->type;

    // Determine element type and get Slice type
    TypeId elem_type = TypeTable::Error;
    bool is_array = false;
    uint32_t array_size = 0;

    if (arr_type != TypeTable::Error && arr_type < ctx_.types.size()) {
        const auto& ti = ctx_.types.get(arr_type);
        if (ti.kind == TypeKind::Array) {
            elem_type = ti.array.element;
            is_array = true;
            array_size = ti.array.count;
        } else if (ti.kind == TypeKind::Ptr || ti.kind == TypeKind::PtrMut) {
            elem_type = ti.ptr.pointee;
        } else if (ti.kind == TypeKind::Struct &&
                   ti.struct_.name.substr(0, 6) == "Slice_" &&
                   ti.struct_.field_count >= 2) {
            // Slicing a slice: s[start..end]
            const auto& data_ti = ctx_.types.get(ti.struct_.fields[0].type);
            if (data_ti.kind == TypeKind::Ptr || data_ti.kind == TypeKind::PtrMut) {
                elem_type = data_ti.ptr.pointee;
            }
        }
    }

    if (elem_type == TypeTable::Error) {
        ctx_.diag.error(expr->loc, "slice operation on non-array/slice type");
        auto* err = ctx_.arena.make<HIRIntLitExpr>();
        err->kind = HIRExpr::Kind::IntLit;
        err->loc = expr->loc;
        err->value = 0;
        err->type = TypeTable::Error;
        return err;
    }

    // Ensure Slice<T> type exists
    std::string slice_name = "Slice_" + std::string(ctx_.types.name(elem_type));
    auto interned = ctx_.strings.intern(slice_name);
    TypeId slice_type;
    auto it = named_types_.find(interned);
    if (it != named_types_.end()) {
        slice_type = it->second;
    } else {
        FieldInfo fields[] = {
            {ctx_.strings.intern("data"), ctx_.types.makePtr(elem_type, false), false, 0},
            {ctx_.strings.intern("len"), TypeTable::U64, false, 8},
        };
        slice_type = ctx_.types.makeStruct(interned, fields);
        named_types_[interned] = slice_type;
    }

    // Build start expression (default 0u64 if omitted)
    HIRExpr* start_expr;
    if (se->start) {
        start_expr = buildExpr(se->start);
    } else {
        auto* zero = ctx_.arena.make<HIRIntLitExpr>();
        zero->kind = HIRExpr::Kind::IntLit;
        zero->loc = expr->loc;
        zero->value = 0;
        zero->type = TypeTable::U64;
        start_expr = zero;
    }

    // Build end expression (default array_size if omitted — only for fixed arrays)
    HIRExpr* end_expr;
    if (se->end) {
        end_expr = buildExpr(se->end);
    } else if (is_array) {
        auto* size_lit = ctx_.arena.make<HIRIntLitExpr>();
        size_lit->kind = HIRExpr::Kind::IntLit;
        size_lit->loc = expr->loc;
        size_lit->value = static_cast<int64_t>(array_size);
        size_lit->type = TypeTable::U64;
        end_expr = size_lit;
    } else {
        ctx_.diag.error(expr->loc, "open-ended slice requires fixed-size array");
        auto* err = ctx_.arena.make<HIRIntLitExpr>();
        err->kind = HIRExpr::Kind::IntLit;
        err->loc = expr->loc;
        err->value = 0;
        err->type = TypeTable::Error;
        return err;
    }

    // Build &arr[start] for the data field
    // For arrays: &arr[start]
    // For slices: s.data + start (pointer arithmetic)
    HIRExpr* data_ptr;
    const auto& arr_ti = ctx_.types.get(arr_type);
    if (arr_ti.kind == TypeKind::Struct &&
        arr_ti.struct_.name.substr(0, 6) == "Slice_") {
        // Slicing a slice: data = s.data + start
        auto* data_access = ctx_.arena.make<HIRFieldAccessExpr>();
        data_access->kind = HIRExpr::Kind::FieldAccess;
        data_access->loc = expr->loc;
        data_access->object = arr_expr;
        data_access->field_name = ctx_.strings.intern("data");
        data_access->type = arr_ti.struct_.fields[0].type;

        // data_ptr = s.data + start (pointer arithmetic via BinOp)
        auto* add = ctx_.arena.make<HIRBinOpExpr>();
        add->kind = HIRExpr::Kind::BinOp;
        add->loc = expr->loc;
        add->op = HIRBinOp::Add;
        add->lhs = data_access;
        add->rhs = start_expr;
        add->type = data_access->type;
        data_ptr = add;
    } else {
        // Array or pointer: &arr[start]
        auto* idx_acc = ctx_.arena.make<HIRIndexAccessExpr>();
        idx_acc->kind = HIRExpr::Kind::IndexAccess;
        idx_acc->loc = expr->loc;
        idx_acc->array = arr_expr;
        idx_acc->index = start_expr;
        idx_acc->type = elem_type;

        auto* addr = ctx_.arena.make<HIRAddrOfExpr>();
        addr->kind = HIRExpr::Kind::AddrOf;
        addr->loc = expr->loc;
        addr->operand = idx_acc;
        addr->is_mutable = false;
        addr->type = ctx_.types.makePtr(elem_type, false);
        data_ptr = addr;
    }

    // Build len = end - start
    auto* len_expr = ctx_.arena.make<HIRBinOpExpr>();
    len_expr->kind = HIRExpr::Kind::BinOp;
    len_expr->loc = expr->loc;
    len_expr->op = HIRBinOp::Sub;
    len_expr->lhs = end_expr;
    len_expr->rhs = start_expr;
    len_expr->type = TypeTable::U64;

    // Build Slice { data: data_ptr, len: len_expr }
    auto* sl = ctx_.arena.make<HIRStructLitExpr>();
    sl->kind = HIRExpr::Kind::StructLit;
    sl->loc = expr->loc;
    sl->struct_name = interned;
    sl->type = slice_type;
    sl->field_count = 2;
    sl->fields = ctx_.arena.makeArray<HIRFieldInit>(2);
    sl->fields[0].name = ctx_.strings.intern("data");
    sl->fields[0].value = data_ptr;
    sl->fields[0].loc = expr->loc;
    sl->fields[1].name = ctx_.strings.intern("len");
    sl->fields[1].value = len_expr;
    sl->fields[1].loc = expr->loc;

    return sl;
}

// ============================================================================
// Inline assembly building
// ============================================================================

HIRExpr* HIRBuilder::buildInlineAsm(const Expr* expr) {
    auto* ia = static_cast<const InlineAsmExpr*>(expr);
    auto* e = ctx_.arena.make<HIRInlineAsmExpr>();
    e->kind = HIRExpr::Kind::InlineAsm;
    e->loc = expr->loc;
    e->type = TypeTable::Unit;
    e->line_count = ia->line_count;
    e->lines = ctx_.arena.makeArray<HIRInlineAsmLine>(ia->line_count);
    for (uint32_t i = 0; i < ia->line_count; ++i) {
        e->lines[i].text = ia->lines[i]->data;
        e->lines[i].length = ia->lines[i]->length;
    }

    // Propagate constraint info
    e->output_count = ia->output_count;
    e->input_count = ia->input_count;
    e->clobber_count = ia->clobber_count;
    if (ia->output_count > 0) {
        e->outputs = ctx_.arena.makeArray<HIRAsmOperand>(ia->output_count);
        for (uint32_t i = 0; i < ia->output_count; ++i) {
            e->outputs[i].constraint = ia->outputs[i].constraint;
            e->outputs[i].var_name = ia->outputs[i].var_name;
        }
    } else { e->outputs = nullptr; }
    if (ia->input_count > 0) {
        e->inputs = ctx_.arena.makeArray<HIRAsmOperand>(ia->input_count);
        for (uint32_t i = 0; i < ia->input_count; ++i) {
            e->inputs[i].constraint = ia->inputs[i].constraint;
            e->inputs[i].var_name = ia->inputs[i].var_name;
        }
    } else { e->inputs = nullptr; }
    if (ia->clobber_count > 0) {
        e->clobbers = ctx_.arena.makeArray<std::string_view>(ia->clobber_count);
        for (uint32_t i = 0; i < ia->clobber_count; ++i)
            e->clobbers[i] = ia->clobbers[i];
    } else { e->clobbers = nullptr; }

    return e;
}

// ============================================================================
// Match building
// ============================================================================

HIRExpr* HIRBuilder::buildMatch(const Expr* expr, std::optional<TypeId> ctx_type) {
    auto* matchE = static_cast<const MatchExpr*>(expr);
    auto* e = ctx_.arena.make<HIRMatchExpr>();
    e->kind = HIRExpr::Kind::Match;
    e->loc = expr->loc;

    e->scrutinee = buildExpr(matchE->scrutinee);
    TypeId scrut_type = e->scrutinee->type;

    if (scrut_type == TypeTable::Error) {
        e->type = TypeTable::Error;
        e->arms = nullptr;
        e->arm_count = 0;
        return e;
    }

    auto saved_locals = local_vars_;

    // Resolve enum/union for scrutinee
    const TypeInfo* scrut_info = &ctx_.types.get(scrut_type);
    bool is_enum = scrut_info->kind == TypeKind::Enum;
    bool is_union = scrut_info->kind == TypeKind::Union;

    TypeId arm_type = TypeTable::Error;
    bool has_catch_all = false;
    bool has_true = false, has_false = false;
    std::unordered_set<std::string_view> covered_variants;

    e->arm_count = matchE->arm_count;
    e->arms = ctx_.arena.makeArray<HIRMatchArm>(matchE->arm_count);

    for (uint32_t i = 0; i < matchE->arm_count; ++i) {
        auto& ast_arm = matchE->arms[i];
        local_vars_ = saved_locals;

        // Build pattern
        HIRPattern* hir_pat = buildPattern(ast_arm.pattern, scrut_type);

        // Track exhaustiveness
        switch (hir_pat->kind) {
            case HIRPattern::Kind::IntLit:
                if (!isIntegerType(scrut_type)) {
                    ctx_.diag.error(ast_arm.pattern->loc,
                        std::string("integer pattern incompatible with scrutinee type ") +
                        ctx_.types.name(scrut_type));
                }
                break;
            case HIRPattern::Kind::Range:
                if (!isIntegerType(scrut_type)) {
                    ctx_.diag.error(ast_arm.pattern->loc,
                        std::string("range pattern incompatible with scrutinee type ") +
                        ctx_.types.name(scrut_type));
                }
                break;
            case HIRPattern::Kind::BoolLit: {
                if (scrut_type != TypeTable::Bool) {
                    ctx_.diag.error(ast_arm.pattern->loc,
                        "bool pattern incompatible with non-bool scrutinee");
                } else {
                    auto* bp = static_cast<HIRBoolLitPattern*>(hir_pat);
                    if (bp->value) has_true = true; else has_false = true;
                }
                break;
            }
            case HIRPattern::Kind::Variable: {
                auto* vp = static_cast<HIRVariablePattern*>(hir_pat);
                // Check if it's actually an enum/union variant
                bool promoted = false;
                if (is_enum) {
                    for (uint32_t v = 0; v < scrut_info->enum_.variant_count; ++v) {
                        if (scrut_info->enum_.names[v] == vp->name) {
                            // Promote to enum pattern
                            auto* ep = ctx_.arena.make<HIREnumPattern>();
                            ep->kind = HIRPattern::Kind::Enum;
                            ep->type = scrut_type;
                            ep->loc = vp->loc;
                            ep->variant_name = vp->name;
                            hir_pat = ep;
                            covered_variants.insert(vp->name);
                            promoted = true;
                            break;
                        }
                    }
                } else if (is_union) {
                    for (uint32_t v = 0; v < scrut_info->union_.variant_count; ++v) {
                        if (scrut_info->union_.variants[v].name == vp->name) {
                            auto* up = ctx_.arena.make<HIRUnionPattern>();
                            up->kind = HIRPattern::Kind::Union;
                            up->type = scrut_type;
                            up->loc = vp->loc;
                            up->variant_name = vp->name;
                            up->inner = nullptr;
                            up->field_bindings = nullptr;
                            up->field_binding_count = 0;
                            hir_pat = up;
                            covered_variants.insert(vp->name);
                            promoted = true;
                            break;
                        }
                    }
                }
                if (!promoted) {
                    local_vars_[vp->name] = scrut_type;
                    if (!ast_arm.guard) has_catch_all = true;
                }
                break;
            }
            case HIRPattern::Kind::Wildcard:
                if (!ast_arm.guard) has_catch_all = true;
                break;
            case HIRPattern::Kind::Enum: {
                auto* ep = static_cast<HIREnumPattern*>(hir_pat);
                covered_variants.insert(ep->variant_name);
                break;
            }
            case HIRPattern::Kind::Union: {
                auto* up = static_cast<HIRUnionPattern*>(hir_pat);
                covered_variants.insert(up->variant_name);
                // Bind inner variable to payload type
                if (is_union && up->inner) {
                    for (uint32_t v = 0; v < scrut_info->union_.variant_count; ++v) {
                        if (scrut_info->union_.variants[v].name == up->variant_name) {
                            TypeId payload_tid = scrut_info->union_.variants[v].payload_type;
                            if (payload_tid != INVALID_TYPE &&
                                up->inner->kind == HIRPattern::Kind::Variable) {
                                auto* inner_vp = static_cast<HIRVariablePattern*>(up->inner);
                                local_vars_[inner_vp->name] = payload_tid;
                            }
                            // Field bindings for struct payload
                            if (up->field_binding_count > 0 && payload_tid != INVALID_TYPE) {
                                const auto& payload_info = ctx_.types.get(payload_tid);
                                if (payload_info.kind == TypeKind::Struct) {
                                    for (uint32_t fb = 0; fb < up->field_binding_count; ++fb) {
                                        for (uint32_t fj = 0; fj < payload_info.struct_.field_count; ++fj) {
                                            if (payload_info.struct_.fields[fj].name == up->field_bindings[fb].field_name) {
                                                local_vars_[up->field_bindings[fb].binding_name] =
                                                    payload_info.struct_.fields[fj].type;
                                                break;
                                            }
                                        }
                                    }
                                }
                            }
                            break;
                        }
                    }
                }
                break;
            }
        }

        // Build guard
        HIRExpr* hir_guard = nullptr;
        if (ast_arm.guard) {
            hir_guard = buildExpr(ast_arm.guard);
            if (hir_guard->type != TypeTable::Error && hir_guard->type != TypeTable::Bool) {
                ctx_.diag.error(ast_arm.guard->loc, "match guard must be bool");
            }
        }

        // Build body
        HIRExpr* hir_body = buildExpr(ast_arm.body, ctx_type);

        e->arms[i] = {hir_pat, hir_guard, hir_body, ast_arm.loc};

        // Track arm type (Never is bottom type, compatible with anything)
        if (hir_body->type != TypeTable::Error) {
            if (arm_type == TypeTable::Error) {
                arm_type = hir_body->type;
            } else if (!typesMatchClosure(hir_body->type, arm_type, ctx_.types, closure_struct_types_)) {
                ctx_.diag.error(ast_arm.body->loc,
                    std::string("match arm type mismatch: expected ") +
                    ctx_.types.name(arm_type) + ", got " + ctx_.types.name(hir_body->type));
            } else {
                arm_type = mergeTypes(arm_type, hir_body->type);
            }
        }
    }

    local_vars_ = saved_locals;

    // Exhaustiveness check
    if (scrut_type == TypeTable::Bool) {
        if (!has_catch_all && !(has_true && has_false)) {
            ctx_.diag.error(matchE->loc, "non-exhaustive match on bool: missing patterns");
        }
    } else if (isIntegerType(scrut_type)) {
        if (!has_catch_all) {
            ctx_.diag.error(matchE->loc,
                "non-exhaustive match on integer: add a wildcard '_' or variable pattern");
        }
    } else if (is_enum && !has_catch_all) {
        for (uint32_t v = 0; v < scrut_info->enum_.variant_count; ++v) {
            if (covered_variants.find(scrut_info->enum_.names[v]) == covered_variants.end()) {
                ctx_.diag.error(matchE->loc,
                    std::string("non-exhaustive match on enum '") +
                    std::string(scrut_info->enum_.name) + "': missing variant '" +
                    std::string(scrut_info->enum_.names[v]) + "'");
            }
        }
    } else if (is_union && !has_catch_all) {
        for (uint32_t v = 0; v < scrut_info->union_.variant_count; ++v) {
            if (covered_variants.find(scrut_info->union_.variants[v].name) == covered_variants.end()) {
                ctx_.diag.error(matchE->loc,
                    std::string("non-exhaustive match on union '") +
                    std::string(scrut_info->union_.name) + "': missing variant '" +
                    std::string(scrut_info->union_.variants[v].name) + "'");
            }
        }
    }

    e->type = arm_type;
    return e;
}

HIRPattern* HIRBuilder::buildPattern(const Pattern* pat, TypeId scrut_type) {
    switch (pat->kind) {
        case Pattern::Kind::IntLit: {
            auto* asp = static_cast<const IntLitPattern*>(pat);
            auto* p = ctx_.arena.make<HIRIntLitPattern>();
            p->kind = HIRPattern::Kind::IntLit;
            p->type = scrut_type;
            p->loc = pat->loc;
            p->value = asp->value;
            return p;
        }
        case Pattern::Kind::BoolLit: {
            auto* asp = static_cast<const BoolLitPattern*>(pat);
            auto* p = ctx_.arena.make<HIRBoolLitPattern>();
            p->kind = HIRPattern::Kind::BoolLit;
            p->type = TypeTable::Bool;
            p->loc = pat->loc;
            p->value = asp->value;
            return p;
        }
        case Pattern::Kind::Wildcard: {
            auto* p = ctx_.arena.make<HIRWildcardPattern>();
            p->kind = HIRPattern::Kind::Wildcard;
            p->type = scrut_type;
            p->loc = pat->loc;
            return p;
        }
        case Pattern::Kind::Variable: {
            auto* asp = static_cast<const VariablePattern*>(pat);
            auto* p = ctx_.arena.make<HIRVariablePattern>();
            p->kind = HIRPattern::Kind::Variable;
            p->type = scrut_type;
            p->loc = pat->loc;
            p->name = ctx_.strings.intern(asp->name);
            return p;
        }
        case Pattern::Kind::Enum: {
            auto* asp = static_cast<const EnumPattern*>(pat);
            auto* p = ctx_.arena.make<HIREnumPattern>();
            p->kind = HIRPattern::Kind::Enum;
            p->type = scrut_type;
            p->loc = pat->loc;
            p->variant_name = ctx_.strings.intern(asp->variant_name);
            return p;
        }
        case Pattern::Kind::Range: {
            auto* asp = static_cast<const RangePattern*>(pat);
            auto* p = ctx_.arena.make<HIRRangePattern>();
            p->kind = HIRPattern::Kind::Range;
            p->type = scrut_type;
            p->loc = pat->loc;
            p->lo = asp->lo;
            p->hi = asp->hi;
            p->inclusive = asp->inclusive;
            return p;
        }
        case Pattern::Kind::Union: {
            auto* asp = static_cast<const UnionPattern*>(pat);
            auto* p = ctx_.arena.make<HIRUnionPattern>();
            p->kind = HIRPattern::Kind::Union;
            p->type = scrut_type;
            p->loc = pat->loc;
            p->variant_name = ctx_.strings.intern(asp->variant_name);
            p->inner = asp->inner ? buildPattern(asp->inner, scrut_type) : nullptr;
            if (asp->field_bindings && asp->field_binding_count > 0) {
                p->field_binding_count = asp->field_binding_count;
                p->field_bindings = ctx_.arena.makeArray<HIRFieldBinding>(asp->field_binding_count);
                for (uint32_t i = 0; i < asp->field_binding_count; ++i) {
                    p->field_bindings[i] = {
                        ctx_.strings.intern(asp->field_bindings[i].field_name),
                        ctx_.strings.intern(asp->field_bindings[i].binding_name),
                        asp->field_bindings[i].loc
                    };
                }
            } else {
                p->field_bindings = nullptr;
                p->field_binding_count = 0;
            }
            return p;
        }
    }
    // Fallback
    auto* p = ctx_.arena.make<HIRWildcardPattern>();
    p->kind = HIRPattern::Kind::Wildcard;
    p->type = scrut_type;
    p->loc = pat->loc;
    return p;
}

// ============================================================================
// Statement building
// ============================================================================

HIRStmt* HIRBuilder::buildStmt(const Stmt* stmt) {
    switch (stmt->kind) {
        case Stmt::Kind::ValDecl: {
            auto* decl = static_cast<const ValDeclStmt*>(stmt);
            bool has_type_annotation = !decl->type.name.empty() ||
                                       decl->type.kind != TypeRef::Kind::Named;
            TypeId expected = has_type_annotation ? resolveType(decl->type) : INVALID_TYPE;
            auto* s = ctx_.arena.make<HIRValDeclStmt>();
            s->kind = HIRStmt::Kind::ValDecl;
            s->loc = stmt->loc;
            s->name = ctx_.strings.intern(decl->name);
            if (has_type_annotation) {
                s->type = expected;
                s->init = buildExpr(decl->init, expected);
            } else {
                s->init = buildExpr(decl->init);
                expected = s->init->type;
                s->type = expected;
            }
            if (s->init->type != TypeTable::Error && s->init->type != expected) {
                // Allow closure struct to be assigned to Fn-typed variable
                if (closure_struct_types_.count(s->init->type) &&
                      expected < ctx_.types.size() &&
                      ctx_.types.get(expected).kind == TypeKind::Fn) {
                    // ok — closure coercion
                } else {
                    s->init = implicitWiden(s->init, expected);
                    if (s->init->type != expected) {
                        ctx_.diag.error(stmt->loc, std::string("val type mismatch: expected ") +
                                    ctx_.types.name(expected) + ", got " + ctx_.types.name(s->init->type));
                    }
                }
            }
            // If init is a closure struct, store the closure type so call sites can unpack
            if (closure_struct_types_.count(s->init->type)) {
                local_vars_[decl->name] = s->init->type;
                s->type = s->init->type;
            } else {
                local_vars_[decl->name] = expected;
            }
            // Track if this val holds a closure (lambda with captures) — legacy local path
            if (s->init->kind == HIRExpr::Kind::FnRef) {
                auto* fnref = static_cast<HIRFnRefExpr*>(s->init);
                if (lambda_captures_.count(fnref->fn_name) &&
                    !lambda_captures_[fnref->fn_name].empty()) {
                    local_lambda_map_[s->name] = fnref->fn_name;
                }
            }
            return s;
        }
        case Stmt::Kind::VarDecl: {
            auto* decl = static_cast<const VarDeclStmt*>(stmt);
            bool has_type_annotation = !decl->type.name.empty() ||
                                       decl->type.kind != TypeRef::Kind::Named;
            TypeId expected = has_type_annotation ? resolveType(decl->type) : INVALID_TYPE;
            auto* s = ctx_.arena.make<HIRVarDeclStmt>();
            s->kind = HIRStmt::Kind::VarDecl;
            s->loc = stmt->loc;
            s->name = ctx_.strings.intern(decl->name);
            if (has_type_annotation) {
                s->type = expected;
                s->init = buildExpr(decl->init, expected);
            } else {
                s->init = buildExpr(decl->init);
                expected = s->init->type;
                s->type = expected;
            }
            if (s->init->type != TypeTable::Error && s->init->type != expected) {
                s->init = implicitWiden(s->init, expected);
                if (s->init->type != expected) {
                    ctx_.diag.error(stmt->loc, std::string("var type mismatch: expected ") +
                                ctx_.types.name(expected) + ", got " + ctx_.types.name(s->init->type));
                }
            }
            local_vars_[decl->name] = expected;
            mutable_vars_.insert(decl->name);
            return s;
        }
        case Stmt::Kind::Assign: {
            auto* assign = static_cast<const AssignStmt*>(stmt);
            auto* s = ctx_.arena.make<HIRAssignStmt>();
            s->kind = HIRStmt::Kind::Assign;
            s->loc = stmt->loc;
            s->name = ctx_.strings.intern(assign->name);

            // Check global variables first
            auto gv_it = global_vars_.find(assign->name);
            if (gv_it != global_vars_.end()) {
                if (!gv_it->second->is_mutable) {
                    ctx_.diag.error(stmt->loc, std::string("cannot assign to immutable global '") +
                                std::string(assign->name) + "'; use 'static var' instead of 'static val'");
                    s->value = buildExpr(assign->value);
                    return s;
                }
                TypeId gtype = gv_it->second->type_id;
                s->value = buildExpr(assign->value, gtype);
                if (s->value->type != TypeTable::Error && s->value->type != gtype) {
                    s->value = implicitWiden(s->value, gtype);
                    if (s->value->type != gtype) {
                        ctx_.diag.error(stmt->loc, std::string("assignment type mismatch: expected ") +
                                    ctx_.types.name(gtype) + ", got " + ctx_.types.name(s->value->type));
                    }
                }
                return s;
            }

            auto it = local_vars_.find(assign->name);
            if (it == local_vars_.end()) {
                ctx_.diag.error(stmt->loc, std::string("undeclared identifier '") +
                            std::string(assign->name) + "'");
                s->value = buildExpr(assign->value);
                return s;
            }
            if (mutable_vars_.find(assign->name) == mutable_vars_.end()) {
                ctx_.diag.error(stmt->loc, std::string("cannot assign to immutable binding '") +
                            std::string(assign->name) + "'; use 'var' instead of 'val'");
                ctx_.diag.note(stmt->loc, "to make this binding mutable, declare it with 'var'");
                s->value = buildExpr(assign->value);
                return s;
            }
            s->value = buildExpr(assign->value, it->second);
            if (s->value->type != TypeTable::Error && s->value->type != it->second) {
                s->value = implicitWiden(s->value, it->second);
                if (s->value->type != it->second) {
                    ctx_.diag.error(stmt->loc, std::string("assignment type mismatch: expected ") +
                                ctx_.types.name(it->second) + ", got " + ctx_.types.name(s->value->type));
                }
            }
            return s;
        }
        case Stmt::Kind::ExprStmt: {
            auto* es = static_cast<const ExprStmt*>(stmt);
            auto* s = ctx_.arena.make<HIRExprStmt>();
            s->kind = HIRStmt::Kind::ExprStmt;
            s->loc = stmt->loc;
            s->expr = buildExpr(es->expr);
            return s;
        }
        case Stmt::Kind::FieldAssign: {
            auto* fas = static_cast<const FieldAssignStmt*>(stmt);
            auto* s = ctx_.arena.make<HIRFieldAssignStmt>();
            s->kind = HIRStmt::Kind::FieldAssign;
            s->loc = stmt->loc;
            s->target = buildExpr(fas->target);

            // Walk chain to check root mutability
            const Expr* root = fas->target;
            while (root->kind == Expr::Kind::FieldAccess) {
                root = static_cast<const FieldAccessExpr*>(root)->object;
            }
            if (root->kind == Expr::Kind::Ident) {
                auto* ident = static_cast<const IdentExpr*>(root);
                if (mutable_vars_.find(ident->name) == mutable_vars_.end()) {
                    ctx_.diag.error(stmt->loc,
                        std::string("cannot assign to field of immutable binding '") +
                        std::string(ident->name) + "'; use 'var' instead of 'val'");
                }
            }

            // Check field mutability via TypeTable
            if (s->target->kind == HIRExpr::Kind::FieldAccess) {
                auto* hfa = static_cast<HIRFieldAccessExpr*>(s->target);
                TypeId obj_type = hfa->object->type;
                if (obj_type != TypeTable::Error) {
                    const auto& obj_info = ctx_.types.get(obj_type);
                    if (obj_info.kind == TypeKind::Struct) {
                        for (uint32_t i = 0; i < obj_info.struct_.field_count; ++i) {
                            if (obj_info.struct_.fields[i].name == hfa->field_name) {
                                if (!obj_info.struct_.fields[i].is_mutable) {
                                    ctx_.diag.error(stmt->loc,
                                        std::string("cannot assign to immutable field '") +
                                        std::string(hfa->field_name) + "' of struct '" +
                                        std::string(obj_info.struct_.name) + "'");
                                }
                                break;
                            }
                        }
                    }
                }
            }

            TypeId target_type = s->target->type;
            s->value = buildExpr(fas->value, target_type);
            if (s->value->type != TypeTable::Error && target_type != TypeTable::Error &&
                s->value->type != target_type) {
                ctx_.diag.error(stmt->loc,
                    std::string("field assignment type mismatch: expected ") +
                    ctx_.types.name(target_type) + ", got " + ctx_.types.name(s->value->type));
            }
            return s;
        }
        case Stmt::Kind::DerefAssign: {
            auto* da = static_cast<const DerefAssignStmt*>(stmt);
            auto* s = ctx_.arena.make<HIRDerefAssignStmt>();
            s->kind = HIRStmt::Kind::DerefAssign;
            s->loc = stmt->loc;
            s->target = buildExpr(da->target);

            // Walk to find root deref and check Ptr<var T>
            HIRExpr* root_hir = s->target;
            while (root_hir->kind == HIRExpr::Kind::FieldAccess) {
                root_hir = static_cast<HIRFieldAccessExpr*>(root_hir)->object;
            }
            if (root_hir->kind == HIRExpr::Kind::Deref) {
                auto* deref = static_cast<HIRDerefExpr*>(root_hir);
                TypeId ptr_type = deref->operand->type;
                if (ptr_type != TypeTable::Error) {
                    const auto& ptr_info = ctx_.types.get(ptr_type);
                    if (ptr_info.kind == TypeKind::Ptr) {
                        ctx_.diag.error(stmt->loc,
                            "cannot write through 'Ptr<T>' (read-only); use 'Ptr<var T>' instead");
                    } else if (ptr_info.kind != TypeKind::PtrMut) {
                        ctx_.diag.error(stmt->loc,
                            std::string("cannot write through non-pointer type '") +
                            ctx_.types.name(ptr_type) + "'");
                    }
                }
            }

            TypeId target_type = s->target->type;
            s->value = buildExpr(da->value, target_type);
            if (s->value->type != TypeTable::Error && target_type != TypeTable::Error &&
                s->value->type != target_type) {
                ctx_.diag.error(stmt->loc,
                    std::string("deref assignment type mismatch: expected ") +
                    ctx_.types.name(target_type) + ", got " + ctx_.types.name(s->value->type));
            }
            return s;
        }
        case Stmt::Kind::Break: {
            auto* bs = static_cast<const BreakStmt*>(stmt);
            auto* brk = ctx_.arena.make<HIRBreakExpr>();
            brk->kind = HIRExpr::Kind::Break;
            brk->loc = stmt->loc;
            brk->type = TypeTable::Unit;
            brk->label = bs->label;
            if (bs->value) {
                brk->value = buildExpr(bs->value);
                brk->type = brk->value->type;
            } else {
                brk->value = nullptr;
            }
            auto* es = ctx_.arena.make<HIRExprStmt>();
            es->kind = HIRStmt::Kind::ExprStmt;
            es->loc = stmt->loc;
            es->expr = brk;
            return es;
        }
        case Stmt::Kind::Continue: {
            auto* cs = static_cast<const ContinueStmt*>(stmt);
            auto* cont = ctx_.arena.make<HIRContinueExpr>();
            cont->kind = HIRExpr::Kind::Continue;
            cont->loc = stmt->loc;
            cont->type = TypeTable::Unit;
            cont->label = cs->label;
            cont->arg_count = cs->arg_count;
            cont->args = ctx_.arena.makeArray<HIRExpr*>(cs->arg_count);
            for (uint32_t i = 0; i < cs->arg_count; ++i) {
                cont->args[i] = buildExpr(cs->args[i]);
            }
            auto* es = ctx_.arena.make<HIRExprStmt>();
            es->kind = HIRStmt::Kind::ExprStmt;
            es->loc = stmt->loc;
            es->expr = cont;
            return es;
        }
        case Stmt::Kind::IndexAssign: {
            auto* ias = static_cast<const IndexAssignStmt*>(stmt);
            auto* s = ctx_.arena.make<HIRIndexAssignStmt>();
            s->kind = HIRStmt::Kind::IndexAssign;
            s->loc = stmt->loc;
            s->array = buildExpr(ias->array);
            s->index = buildExpr(ias->index);
            s->value = buildExpr(ias->value);
            return s;
        }
    }
    // unreachable
    auto* s = ctx_.arena.make<HIRExprStmt>();
    s->kind = HIRStmt::Kind::ExprStmt;
    s->loc = stmt->loc;
    s->expr = errorExpr(stmt->loc);
    return s;
}

// ============================================================================
// Lambda building (closure conversion — lift to top-level function)
// ============================================================================

HIRExpr* HIRBuilder::buildLambda(const Expr* expr, std::optional<TypeId> ctx_type) {
    auto* lam = static_cast<const LambdaExpr*>(expr);

    // Determine param types from context type (Fn type) or explicit annotations
    std::vector<TypeId> param_types;
    TypeId return_type = INVALID_TYPE;

    // If context type is an Fn type, use it to infer param/return types
    const TypeInfo* ctx_fn_info = nullptr;
    if (ctx_type && *ctx_type < ctx_.types.size() &&
        ctx_.types.get(*ctx_type).kind == TypeKind::Fn) {
        ctx_fn_info = &ctx_.types.get(*ctx_type);
        if (ctx_fn_info->fn.param_count != lam->param_count) {
            ctx_.diag.error(expr->loc,
                std::string("lambda has ") + std::to_string(lam->param_count) +
                " parameters, but context expects " +
                std::to_string(ctx_fn_info->fn.param_count));
            return errorExpr(expr->loc);
        }
    }

    for (uint32_t i = 0; i < lam->param_count; ++i) {
        if (!lam->params[i].type.name.empty()) {
            // Explicit type annotation
            param_types.push_back(resolveType(lam->params[i].type));
        } else if (ctx_fn_info && i < ctx_fn_info->fn.param_count) {
            // Infer from context
            param_types.push_back(ctx_fn_info->fn.params[i]);
        } else {
            ctx_.diag.error(lam->params[i].loc,
                std::string("cannot infer type for lambda parameter '") +
                std::string(lam->params[i].name) + "'");
            param_types.push_back(TypeTable::Error);
        }
    }

    if (ctx_fn_info) {
        return_type = ctx_fn_info->fn.return_type;
    }

    // Generate a unique name for the lifted function
    std::string fn_name = "__lambda_" + std::to_string(lambda_counter_++);
    auto interned_name = ctx_.strings.intern(fn_name);

    // Build the lifted HIRFnDecl
    auto* hfn = ctx_.arena.make<HIRFnDecl>();
    hfn->name = interned_name;
    hfn->param_count = lam->param_count;
    hfn->params = ctx_.arena.makeArray<HIRParam>(lam->param_count);
    hfn->is_intrinsic = false;
    hfn->is_const = false;
    hfn->is_naked = false;
    hfn->is_interrupt = false;
    hfn->is_pub = false;     // lambdas are always private
    hfn->is_extern = false;
    hfn->is_variadic = false;
    hfn->is_weak = false;
    hfn->is_recursive = false;
    hfn->is_tail_recursive = false;
    hfn->purity = 0; // will be set by purity pass
    hfn->loc = expr->loc;
    hfn->type_param_count = 0;
    hfn->type_params = nullptr;

    // Save outer scope state
    auto saved_locals = local_vars_;
    auto saved_mutables = mutable_vars_;
    auto saved_return = current_return_type_;
    auto saved_in_lambda = in_lambda_;
    auto saved_outer_locals = outer_locals_;
    auto saved_outer_mutables = outer_mutables_;
    auto saved_captures = current_captures_;

    // Set up capture tracking: remember outer scope, start with clean lambda scope
    in_lambda_ = true;
    outer_locals_ = saved_locals;
    outer_mutables_ = saved_mutables;
    current_captures_.clear();
    local_vars_.clear();
    mutable_vars_.clear();

    // Lambda params are lambda-local
    for (uint32_t i = 0; i < lam->param_count; ++i) {
        auto param_name = ctx_.strings.intern(lam->params[i].name);
        local_vars_[param_name] = param_types[i];
    }

    // Set return type for type checking inside the body
    current_return_type_ = return_type;

    // Build body — buildIdent will detect captures from outer_locals_
    hfn->body = buildExpr(lam->body, return_type != INVALID_TYPE ? std::optional<TypeId>(return_type) : std::nullopt);

    // Sort captures by name for deterministic ordering
    std::sort(current_captures_.begin(), current_captures_.end(),
              [](const CapturedVar& a, const CapturedVar& b) { return a.name < b.name; });

    // Build the lifted function's actual params: lambda params + capture params
    uint32_t total_params = lam->param_count + static_cast<uint32_t>(current_captures_.size());
    hfn->param_count = total_params;
    hfn->params = ctx_.arena.makeArray<HIRParam>(total_params);

    for (uint32_t i = 0; i < lam->param_count; ++i) {
        auto param_name = ctx_.strings.intern(lam->params[i].name);
        hfn->params[i].name = param_name;
        hfn->params[i].type = param_types[i];
        hfn->params[i].loc = lam->params[i].loc;
    }
    for (uint32_t i = 0; i < current_captures_.size(); ++i) {
        hfn->params[lam->param_count + i].name = current_captures_[i].name;
        hfn->params[lam->param_count + i].type = current_captures_[i].type;
        hfn->params[lam->param_count + i].loc = expr->loc;
    }

    // Store capture info for use at call sites
    auto captures = current_captures_;
    lambda_captures_[interned_name] = captures;

    // Infer return type from body if not provided
    if (return_type == INVALID_TYPE && hfn->body) {
        return_type = hfn->body->type;
    }
    hfn->return_type = return_type;

    // Check body type matches return type
    if (hfn->body && hfn->body->type != TypeTable::Error &&
        return_type != INVALID_TYPE && return_type != TypeTable::Error &&
        !typesMatch(hfn->body->type, return_type)) {
        ctx_.diag.error(expr->loc,
            std::string("lambda body type ") + ctx_.types.name(hfn->body->type) +
            " does not match expected return type " + ctx_.types.name(return_type));
    }

    // Restore outer scope
    local_vars_ = saved_locals;
    mutable_vars_ = saved_mutables;
    current_return_type_ = saved_return;
    in_lambda_ = saved_in_lambda;
    outer_locals_ = saved_outer_locals;
    outer_mutables_ = saved_outer_mutables;
    current_captures_ = saved_captures;

    // Register the lifted function with ALL params (including captures) in fn_table_
    FnSig sig;
    sig.name = interned_name;
    sig.param_types = param_types;
    for (auto& cap : captures) sig.param_types.push_back(cap.type);
    sig.return_type = return_type;
    fn_table_[interned_name] = sig;

    // Add to lifted lambdas list
    lifted_lambdas_.push_back(hfn);

    // Build the user-visible fn type (without captures)
    auto* tid_params = ctx_.arena.makeArray<TypeId>(param_types.size());
    for (size_t i = 0; i < param_types.size(); ++i) tid_params[i] = param_types[i];
    TypeId fn_type = ctx_.types.makeFn(
        std::span<const TypeId>(tid_params, param_types.size()), return_type);

    // If there are captures, create a closure struct: { __fn: fn_ptr, cap1: T1, ... }
    // This allows the closure to be returned from functions and passed around.
    if (!captures.empty()) {
        // Build closure struct type
        std::string clos_name = "__closure_" + std::to_string(lambda_counter_ - 1);
        auto interned_clos = ctx_.strings.intern(clos_name);

        uint32_t field_count = 1 + static_cast<uint32_t>(captures.size());
        std::vector<FieldInfo> fields;
        fields.reserve(field_count);

        // Field 0: __fn (the function pointer)
        auto fn_field_name = ctx_.strings.intern("__fn");
        fields.push_back({fn_field_name, fn_type, false, -1});

        // Fields 1..N: captured variables
        for (auto& cap : captures) {
            fields.push_back({cap.name, cap.type, cap.is_mutable, -1});
        }

        TypeId closure_type = ctx_.types.makeStruct(
            interned_clos, std::span<const FieldInfo>(fields));

        // Track this as a closure struct for call-site unpacking
        closure_struct_types_.insert(closure_type);
        closure_fn_names_[closure_type] = interned_name;

        // Create a struct literal with fn_ptr + capture values
        auto* slit = ctx_.arena.make<HIRStructLitExpr>();
        slit->kind = HIRExpr::Kind::StructLit;
        slit->loc = expr->loc;
        slit->struct_name = interned_clos;
        slit->field_count = field_count;
        slit->fields = ctx_.arena.makeArray<HIRFieldInit>(field_count);

        // __fn field: function pointer
        auto* fn_ref = ctx_.arena.make<HIRFnRefExpr>();
        fn_ref->kind = HIRExpr::Kind::FnRef;
        fn_ref->loc = expr->loc;
        fn_ref->fn_name = interned_name;
        fn_ref->type = fn_type;
        slit->fields[0].name = fn_field_name;
        slit->fields[0].value = fn_ref;
        slit->fields[0].loc = expr->loc;

        // Capture fields: reference to outer-scope variables
        for (uint32_t i = 0; i < captures.size(); ++i) {
            auto* cap_ref = ctx_.arena.make<HIRIdentExpr>();
            cap_ref->kind = HIRExpr::Kind::Ident;
            cap_ref->loc = expr->loc;
            cap_ref->name = captures[i].name;
            cap_ref->type = captures[i].type;
            slit->fields[1 + i].name = captures[i].name;
            slit->fields[1 + i].value = cap_ref;
            slit->fields[1 + i].loc = expr->loc;
        }

        // The struct type must match what the context expects. If context is Fn type,
        // we store the closure struct type but allow assignment (coerce at type check).
        slit->type = closure_type;

        // Register in named_types_ so it's recognized
        named_types_[interned_clos] = closure_type;

        return slit;
    }

    // No captures: return a plain FnRef (bare function pointer)
    auto* ref = ctx_.arena.make<HIRFnRefExpr>();
    ref->kind = HIRExpr::Kind::FnRef;
    ref->loc = expr->loc;
    ref->fn_name = interned_name;
    ref->type = fn_type;
    return ref;
}

// ============================================================================
// Trait/Impl registration (static dispatch)
// ============================================================================

void HIRBuilder::registerTraits(const Module* ast) {
    for (uint32_t i = 0; i < ast->trait_count; ++i) {
        auto* td = ast->traits[i];
        TraitInfo info;
        info.name = ctx_.strings.intern(td->name);
        for (uint32_t j = 0; j < td->method_count; ++j) {
            info.method_names.push_back(ctx_.strings.intern(td->methods[j].name));
            // Parse effect annotations on trait methods
            EffectSet effects = EFFECT_NONE;
            for (uint32_t k = 0; k < td->methods[j].effect_count; ++k) {
                Effect eff;
                if (parseEffectName(td->methods[j].effect_names[k], eff)) {
                    effects = addEffect(effects, eff);
                }
            }
            info.method_effects.push_back(effects);
        }
        trait_table_[info.name] = info;
    }
}

void HIRBuilder::registerImpls(const Module* ast) {
    for (uint32_t i = 0; i < ast->impl_count; ++i) {
        auto* id = ast->impls[i];
        TypeId target_type = resolveType(id->target_type);

        // Get the type name for mangling
        std::string_view type_name = actualTypeName(target_type, ctx_.types);

        // Register each impl method as a mangled free function
        for (uint32_t j = 0; j < id->method_count; ++j) {
            auto* fn = id->methods[j];
            auto method_name = ctx_.strings.intern(fn->name);

            // Mangled name: TypeName_methodName
            std::string mangled = std::string(type_name) + "_" + std::string(method_name);
            char* buf = ctx_.arena.makeArray<char>(mangled.size());
            std::memcpy(buf, mangled.data(), mangled.size());
            auto interned_mangled = ctx_.strings.intern(std::string_view(buf, mangled.size()));

            // Register in fn_table_ with mangled name
            FnSig sig;
            sig.name = interned_mangled;
            sig.return_type = resolveType(fn->return_type);
            for (uint32_t k = 0; k < fn->param_count; ++k) {
                sig.param_types.push_back(resolveType(fn->params[k].type));
            }
            fn_table_[interned_mangled] = sig;

            // Store in impl_table_ for method resolution
            impl_table_[type_name].methods[method_name] = interned_mangled;
        }
    }
}

std::string_view HIRBuilder::resolveMethod(TypeId type, std::string_view method) const {
    std::string_view type_name = actualTypeName(type, ctx_.types);
    auto it = impl_table_.find(type_name);
    if (it == impl_table_.end()) return {};
    auto mit = it->second.methods.find(method);
    if (mit == it->second.methods.end()) return {};
    return mit->second;
}

HIRExpr* HIRBuilder::buildMethodCall(const Expr* expr) {
    auto* mc = static_cast<const MethodCallExpr*>(expr);

    // Build the object expression first to get its type
    HIRExpr* obj = buildExpr(mc->object);
    if (obj->type == TypeTable::Error) return errorExpr(expr->loc);

    // Check if this is a dyn Trait method call → dispatch through vtable
    if (obj->type < ctx_.types.size() &&
        ctx_.types.get(obj->type).kind == TypeKind::DynTrait) {
        const auto& dyn_info = ctx_.types.get(obj->type).dyn_trait;
        auto method_name = ctx_.strings.intern(mc->method_name);

        // Find method slot index in trait
        int32_t slot = -1;
        for (uint32_t i = 0; i < dyn_info.method_count; ++i) {
            if (dyn_info.method_names[i] == method_name) {
                slot = static_cast<int32_t>(i);
                break;
            }
        }
        if (slot < 0) {
            ctx_.diag.error(expr->loc, std::string("trait '") +
                std::string(dyn_info.trait_name) + "' has no method '" +
                std::string(mc->method_name) + "'");
            return errorExpr(expr->loc);
        }

        // Look up trait info to get the method signature (from the first impl)
        auto trait_it = trait_table_.find(dyn_info.trait_name);
        if (trait_it == trait_table_.end()) return errorExpr(expr->loc);

        // Extract __data pointer (field at offset 0)
        auto* data_access = ctx_.arena.make<HIRFieldAccessExpr>();
        data_access->kind = HIRExpr::Kind::FieldAccess;
        data_access->loc = expr->loc;
        data_access->object = obj;
        data_access->field_name = ctx_.strings.intern("__data");
        data_access->type = ctx_.types.makePtr(TypeTable::U8, false);

        // Extract __vtable pointer (field at offset 8)
        auto* vtbl_access = ctx_.arena.make<HIRFieldAccessExpr>();
        vtbl_access->kind = HIRExpr::Kind::FieldAccess;
        vtbl_access->loc = expr->loc;
        vtbl_access->object = obj;
        vtbl_access->field_name = ctx_.strings.intern("__vtable");
        vtbl_access->type = ctx_.types.makePtr(TypeTable::U64, false);

        // Load fn pointer from vtable[slot]
        auto* slot_idx = ctx_.arena.make<HIRIntLitExpr>();
        slot_idx->kind = HIRExpr::Kind::IntLit;
        slot_idx->loc = expr->loc;
        slot_idx->value = slot;
        slot_idx->type = TypeTable::I64;

        auto* fn_ptr_load = ctx_.arena.make<HIRIndexAccessExpr>();
        fn_ptr_load->kind = HIRExpr::Kind::IndexAccess;
        fn_ptr_load->loc = expr->loc;
        fn_ptr_load->array = vtbl_access;
        fn_ptr_load->index = slot_idx;
        fn_ptr_load->type = TypeTable::U64;  // raw fn pointer

        // Cast fn_ptr to appropriate Fn type
        // For now, we treat it as a raw fn pointer; the indirect call will use it
        auto* fn_cast = ctx_.arena.make<HIRCastExpr>();
        fn_cast->kind = HIRExpr::Kind::Cast;
        fn_cast->loc = expr->loc;
        fn_cast->operand = fn_ptr_load;

        // Determine the function signature: self:Ptr<u8> + explicit args → return_type
        // We need to look up any impl to get param/return types
        // Use the trait's first impl method to infer the return type
        TypeId return_type = TypeTable::Unit;
        std::vector<TypeId> param_types;
        param_types.push_back(ctx_.types.makePtr(TypeTable::U8, false)); // self as Ptr<u8>
        // Try to find the return type from any concrete impl
        for (auto& [tname, impl] : impl_table_) {
            auto mit = impl.methods.find(method_name);
            if (mit != impl.methods.end()) {
                auto fit = fn_table_.find(mit->second);
                if (fit != fn_table_.end()) {
                    return_type = fit->second.return_type;
                    // Get non-self param types
                    for (uint32_t i = 1; i < fit->second.param_types.size(); ++i) {
                        param_types.push_back(fit->second.param_types[i]);
                    }
                    break;
                }
            }
        }

        TypeId fn_type = ctx_.types.makeFn(param_types, return_type);
        fn_cast->target_type = fn_type;
        fn_cast->type = fn_type;

        // Build indirect call: fn_ptr(data_ptr, args...)
        auto* call = ctx_.arena.make<HIRCallIndirectExpr>();
        call->kind = HIRExpr::Kind::CallIndirect;
        call->loc = expr->loc;
        call->callee = fn_cast;
        call->is_tail_call = false;
        call->arg_count = 1 + mc->arg_count;
        call->args = ctx_.arena.makeArray<HIRExpr*>(call->arg_count);
        call->args[0] = data_access;  // self = data ptr
        for (uint32_t i = 0; i < mc->arg_count; ++i) {
            call->args[i + 1] = buildExpr(mc->args[i]);
        }
        call->type = return_type;
        return call;
    }

    // Resolve the method
    auto method_name = ctx_.strings.intern(mc->method_name);
    auto mangled = resolveMethod(obj->type, method_name);
    if (mangled.empty()) {
        ctx_.diag.error(expr->loc, std::string("type '") +
            std::string(ctx_.types.name(obj->type)) + "' has no method '" +
            std::string(mc->method_name) + "'");
        return errorExpr(expr->loc);
    }

    // Look up the mangled function signature
    auto it = fn_table_.find(mangled);
    if (it == fn_table_.end()) {
        ctx_.diag.error(expr->loc, std::string("internal error: mangled method '") +
            std::string(mangled) + "' not found in fn_table");
        return errorExpr(expr->loc);
    }

    const FnSig& sig = it->second;

    // Build call: TypeName_method(self, args...)
    // First param is self (the object), rest are the explicit args
    uint32_t total_args = 1 + mc->arg_count;
    if (total_args != sig.param_types.size()) {
        ctx_.diag.error(expr->loc, std::string("method '") +
            std::string(mc->method_name) + "' expects " +
            std::to_string(sig.param_types.size()) + " arguments (including self), got " +
            std::to_string(total_args));
        return errorExpr(expr->loc);
    }

    auto* call = ctx_.arena.make<HIRCallExpr>();
    call->kind = HIRExpr::Kind::Call;
    call->loc = expr->loc;
    call->callee = mangled;
    call->is_tail_call = false;
    call->arg_count = total_args;
    call->args = ctx_.arena.makeArray<HIRExpr*>(total_args);

    // First arg is self (the object)
    call->args[0] = obj;
    if (obj->type != sig.param_types[0]) {
        ctx_.diag.error(mc->object->loc, std::string("self parameter type mismatch: expected ") +
            std::string(ctx_.types.name(sig.param_types[0])) + ", got " +
            std::string(ctx_.types.name(obj->type)));
    }

    // Build remaining args
    for (uint32_t i = 0; i < mc->arg_count; ++i) {
        TypeId expected = sig.param_types[i + 1];
        call->args[i + 1] = buildExpr(mc->args[i], expected);
        if (call->args[i + 1]->type != TypeTable::Error && call->args[i + 1]->type != expected) {
            ctx_.diag.error(mc->args[i]->loc, std::string("argument type mismatch: expected ") +
                std::string(ctx_.types.name(expected)) + ", got " +
                std::string(ctx_.types.name(call->args[i + 1]->type)));
        }
    }

    call->type = sig.return_type;
    return call;
}

HIRExpr* HIRBuilder::buildTry(const Expr* expr) {
    auto* te = static_cast<const TryExpr*>(expr);
    HIRExpr* operand = buildExpr(te->operand);
    if (operand->type == TypeTable::Error) return errorExpr(expr->loc);

    TypeId op_type = operand->type;
    if (op_type >= ctx_.types.size()) {
        ctx_.diag.error(expr->loc, "'?' operand has invalid type");
        return errorExpr(expr->loc);
    }

    const auto& info = ctx_.types.get(op_type);
    if (info.kind != TypeKind::Union) {
        ctx_.diag.error(expr->loc, "'?' operator requires a union type (e.g. Result<T, E>)");
        return errorExpr(expr->loc);
    }

    // Find Ok and Err variants
    int ok_idx = -1, err_idx = -1;
    for (uint32_t i = 0; i < info.union_.variant_count; ++i) {
        if (info.union_.variants[i].name == "Ok") ok_idx = static_cast<int>(i);
        if (info.union_.variants[i].name == "Err") err_idx = static_cast<int>(i);
    }

    if (ok_idx < 0 || err_idx < 0) {
        ctx_.diag.error(expr->loc, "'?' operator requires a union with Ok and Err variants");
        return errorExpr(expr->loc);
    }

    TypeId ok_payload = info.union_.variants[ok_idx].payload_type;
    TypeId err_payload = info.union_.variants[err_idx].payload_type;

    // Check that the current function returns a compatible Result type
    if (current_return_type_ == INVALID_TYPE) {
        ctx_.diag.error(expr->loc, "'?' operator used outside a function");
        return errorExpr(expr->loc);
    }

    TypeId ret_type = current_return_type_;
    if (ret_type < ctx_.types.size()) {
        const auto& ret_info = ctx_.types.get(ret_type);
        if (ret_info.kind != TypeKind::Union) {
            ctx_.diag.error(expr->loc,
                "function must return a Result/union type to use '?'");
            return errorExpr(expr->loc);
        }

        // Find Err variant in return type to check compatibility
        bool found_err = false;
        for (uint32_t i = 0; i < ret_info.union_.variant_count; ++i) {
            if (ret_info.union_.variants[i].name == "Err" &&
                ret_info.union_.variants[i].payload_type == err_payload) {
                found_err = true;
                break;
            }
        }
        if (!found_err) {
            ctx_.diag.error(expr->loc,
                "error type of '?' does not match function return type's Err variant");
            return errorExpr(expr->loc);
        }
    }

    // Synthesize: match operand { Ok(v) => v, Err(e) => return RetType::Err(e) }
    auto* match = ctx_.arena.make<HIRMatchExpr>();
    match->kind = HIRExpr::Kind::Match;
    match->loc = expr->loc;
    match->scrutinee = operand;
    match->arm_count = 2;
    match->arms = ctx_.arena.makeArray<HIRMatchArm>(2);
    match->type = ok_payload;

    // Arm 0: Ok(v) => v
    {
        auto ok_name = ctx_.strings.intern("__try_ok");
        auto* pat = ctx_.arena.make<HIRUnionPattern>();
        pat->kind = HIRPattern::Kind::Union;
        pat->variant_name = ctx_.strings.intern("Ok");
        pat->field_bindings = nullptr;
        pat->field_binding_count = 0;
        if (ok_payload != INVALID_TYPE) {
            auto* inner = ctx_.arena.make<HIRVariablePattern>();
            inner->kind = HIRPattern::Kind::Variable;
            inner->name = ok_name;
            pat->inner = inner;
        } else {
            pat->inner = nullptr;
        }

        auto* body = ctx_.arena.make<HIRIdentExpr>();
        body->kind = HIRExpr::Kind::Ident;
        body->loc = expr->loc;
        body->name = ok_name;
        body->type = ok_payload;

        match->arms[0].pattern = pat;
        match->arms[0].guard = nullptr;
        match->arms[0].body = body;
        match->arms[0].loc = expr->loc;
    }

    // Arm 1: Err(e) => return RetType::Err(e)
    {
        auto err_name = ctx_.strings.intern("__try_err");
        auto* pat = ctx_.arena.make<HIRUnionPattern>();
        pat->kind = HIRPattern::Kind::Union;
        pat->variant_name = ctx_.strings.intern("Err");
        pat->field_bindings = nullptr;
        pat->field_binding_count = 0;
        if (err_payload != INVALID_TYPE) {
            auto* inner = ctx_.arena.make<HIRVariablePattern>();
            inner->kind = HIRPattern::Kind::Variable;
            inner->name = err_name;
            pat->inner = inner;
        } else {
            pat->inner = nullptr;
        }

        // Build: return RetType::Err(e)
        auto* err_val = ctx_.arena.make<HIRIdentExpr>();
        err_val->kind = HIRExpr::Kind::Ident;
        err_val->loc = expr->loc;
        err_val->name = err_name;
        err_val->type = err_payload;

        auto* err_wrap = ctx_.arena.make<HIRUnionVariantExpr>();
        err_wrap->kind = HIRExpr::Kind::UnionVariant;
        err_wrap->loc = expr->loc;
        err_wrap->union_name = ctx_.types.get(ret_type).union_.name;
        err_wrap->variant_name = ctx_.strings.intern("Err");
        err_wrap->payload = err_val;
        err_wrap->type = ret_type;

        auto* ret = ctx_.arena.make<HIRReturnExpr>();
        ret->kind = HIRExpr::Kind::Return;
        ret->loc = expr->loc;
        ret->value = err_wrap;
        ret->type = ret_type;

        match->arms[1].pattern = pat;
        match->arms[1].guard = nullptr;
        match->arms[1].body = ret;
        match->arms[1].loc = expr->loc;
    }

    return match;
}

} // namespace kern
