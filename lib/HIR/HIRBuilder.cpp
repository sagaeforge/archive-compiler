#include "kern/hir/HIRBuilder.h"
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

// Never (!) is the bottom type — compatible with any other type.
static bool typesMatch(TypeId a, TypeId b) {
    if (a == b) return true;
    if (a == TypeTable::Never || b == TypeTable::Never) return true;
    return false;
}

// When merging two branch types, pick the non-Never type.
static TypeId mergeTypes(TypeId a, TypeId b) {
    if (a == TypeTable::Never) return b;
    if (b == TypeTable::Never) return a;
    return a; // assumes they match
}

// Constant-fold an HIR expression to an integer value (for static_assert).
// Returns true if successfully folded, storing result in *out.
static bool constEvalInt(HIRExpr* expr, int64_t* out) {
    if (expr->kind == HIRExpr::Kind::IntLit) {
        *out = static_cast<HIRIntLitExpr*>(expr)->value;
        return true;
    }
    if (expr->kind == HIRExpr::Kind::BoolLit) {
        *out = static_cast<HIRBoolLitExpr*>(expr)->value ? 1 : 0;
        return true;
    }
    if (expr->kind == HIRExpr::Kind::BinOp) {
        auto* bin = static_cast<HIRBinOpExpr*>(expr);
        int64_t lv, rv;
        if (!constEvalInt(bin->lhs, &lv) || !constEvalInt(bin->rhs, &rv))
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
        }
        return false;
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
                fields.push_back({ctx_.strings.intern(f.name), ft, f.is_mutable, -1});
            }

            TypeId tid = ctx_.types.makeStruct(interned_mangled, fields,
                                                sd->is_packed, sd->explicit_align);
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

            TypeId tid = ctx_.types.makeUnion(interned_mangled, variants);
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
    // First pass: register names so nested references resolve
    for (uint32_t i = 0; i < ast->struct_count; ++i) {
        auto* sd = ast->structs[i];
        if (sd->type_param_count > 0) {
            // Generic struct — store as template, don't register as concrete type
            generic_structs_[sd->name] = sd;
            continue;
        }
        // Placeholder — will be replaced with computed layout
        named_types_[sd->name] = INVALID_TYPE;
    }

    // Second pass: compute layouts and register proper TypeIds
    for (uint32_t i = 0; i < ast->struct_count; ++i) {
        auto* sd = ast->structs[i];
        if (sd->type_param_count > 0) continue; // skip generic structs
        std::vector<FieldInfo> fields;

        for (uint32_t j = 0; j < sd->field_count; ++j) {
            auto& f = sd->fields[j];
            TypeId ft = resolveType(f.type);
            fields.push_back({ctx_.strings.intern(f.name), ft, f.is_mutable, -1});
        }

        TypeId tid = ctx_.types.makeStruct(ctx_.strings.intern(sd->name), fields,
                                            sd->is_packed, sd->explicit_align);
        named_types_[sd->name] = tid;
    }
}

void HIRBuilder::registerEnumDecls(const Module* ast) {
    for (uint32_t i = 0; i < ast->enum_count; ++i) {
        auto* ed = ast->enums[i];
        std::vector<std::string_view> names;
        std::vector<int64_t> values;
        for (uint32_t j = 0; j < ed->variant_count; ++j) {
            names.push_back(ctx_.strings.intern(ed->variants[j].name));
            values.push_back(static_cast<int64_t>(j));
        }
        TypeId tid = ctx_.types.makeEnum(ctx_.strings.intern(ed->name), names, values);
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
        TypeId tid = ctx_.types.makeUnion(ctx_.strings.intern(ud->name), variants);
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

    // Evaluate static_assert declarations
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

    auto* mod = ctx_.arena.make<HIRModule>();

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

    // Functions
    lifted_lambdas_.clear();
    lambda_counter_ = 0;

    // First pass: build all declared functions (may produce lifted lambdas)
    auto* ast_fns = ctx_.arena.makeArray<HIRFnDecl*>(ast->fn_count);
    for (uint32_t i = 0; i < ast->fn_count; ++i) {
        ast_fns[i] = buildFn(ast->functions[i]);
    }

    // Build impl methods as mangled functions
    std::vector<HIRFnDecl*> impl_fns;
    for (uint32_t i = 0; i < ast->impl_count; ++i) {
        auto* id = ast->impls[i];
        TypeId target_type = resolveType(id->target_type);
        std::string_view type_name = ctx_.types.name(target_type);
        for (uint32_t j = 0; j < id->method_count; ++j) {
            auto* fn = id->methods[j];
            auto method_name = ctx_.strings.intern(fn->name);
            // Build mangled name
            std::string mangled = std::string(type_name) + "_" + std::string(method_name);
            char* buf = ctx_.arena.makeArray<char>(mangled.size());
            std::memcpy(buf, mangled.data(), mangled.size());
            auto interned_mangled = ctx_.strings.intern(std::string_view(buf, mangled.size()));

            // Build the function with the mangled name
            // Temporarily rename the FnDecl so buildFn uses the mangled name
            auto original_name = fn->name;
            fn->name = interned_mangled;
            HIRFnDecl* hfn = buildFn(fn);
            fn->name = original_name;  // restore
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
                // Type generic: create a TypeVar
                TypeInfo ti{};
                ti.kind = TypeKind::TypeVar;
                ti.type_var.name = interned;
                TypeId tv_id = ctx_.types.add(ti);
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
    hfn->is_naked = fn->is_naked;
    hfn->is_interrupt = fn->is_interrupt;
    hfn->section_name = fn->section_name;
    hfn->loc = fn->loc;

    // Register parameters
    for (uint32_t i = 0; i < fn->param_count; ++i) {
        TypeId pt = resolveType(fn->params[i].type);
        auto interned_name = ctx_.strings.intern(fn->params[i].name);
        hfn->params[i] = {interned_name, pt, fn->params[i].loc};
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

    if (fn->is_intrinsic) {
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
        !typesMatch(hfn->body->type, current_return_type_)) {
        ctx_.diag.error(fn->loc, std::string("function '") + std::string(fn->name) +
                    "' declared to return " + ctx_.types.name(current_return_type_) +
                    " but body has type " + ctx_.types.name(hfn->body->type));
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
        case Expr::Kind::InlineAsm:  return buildInlineAsm(expr);
        case Expr::Kind::ArrayLit:   return buildArrayLit(expr);
        case Expr::Kind::IndexAccess:return buildIndexAccess(expr);
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
        case Expr::Kind::Lambda:
            return buildLambda(expr, ctx_type);
        case Expr::Kind::MethodCall:
            return buildMethodCall(expr);
    }
    return errorExpr(expr->loc);
}

HIRExpr* HIRBuilder::buildIntLit(const Expr* expr, std::optional<TypeId> ctx_type) {
    auto* lit = static_cast<const IntLitExpr*>(expr);
    auto* e = ctx_.arena.make<HIRIntLitExpr>();
    e->kind = HIRExpr::Kind::IntLit;
    e->loc = expr->loc;
    e->value = lit->value;

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
    }
    return HIRBinOp::Add;
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
                ctx_.diag.error(expr->loc, "comparison operators require same-type operands");
                e->type = TypeTable::Error;
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

    // Check if callee is a local variable (indirect call through function pointer)
    auto local_it = local_vars_.find(call->callee);
    if (local_it != local_vars_.end()) {
        TypeId var_type = local_it->second;
        if (var_type < ctx_.types.size() && ctx_.types.get(var_type).kind == TypeKind::Fn) {
            auto& ti = ctx_.types.get(var_type);
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
                    ctx_.diag.error(call->args[i]->loc,
                        std::string("argument type mismatch: expected ") +
                        ctx_.types.name(expected) + ", got " +
                        ctx_.types.name(e->args[i]->type));
                }
            }
            e->type = ti.fn.return_type;
            return e;
        }
    }

    // Direct call
    auto* e = ctx_.arena.make<HIRCallExpr>();
    e->kind = HIRExpr::Kind::Call;
    e->loc = expr->loc;
    e->callee = ctx_.strings.intern(call->callee);
    e->is_tail_call = false;

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

    // Check if this is a generic function call (any param is TypeVar)
    bool is_generic_call = false;
    for (size_t i = 0; i < sig.param_types.size(); ++i) {
        if (ctx_.types.get(sig.param_types[i]).kind == TypeKind::TypeVar) {
            is_generic_call = true;
            break;
        }
    }
    if (!is_generic_call && ctx_.types.get(sig.return_type).kind == TypeKind::TypeVar) {
        is_generic_call = true;
    }

    // Build type substitution map for generic calls
    std::unordered_map<TypeId, TypeId> type_subst;

    for (uint32_t i = 0; i < call->arg_count; ++i) {
        TypeId expected = sig.param_types[i];
        // For generic calls, don't pass TypeVar as context type
        std::optional<TypeId> ctx_t;
        if (!is_generic_call || ctx_.types.get(expected).kind != TypeKind::TypeVar) {
            ctx_t = expected;
        }
        e->args[i] = buildExpr(call->args[i], ctx_t);

        if (is_generic_call && ctx_.types.get(expected).kind == TypeKind::TypeVar) {
            // Infer: TypeVar → actual arg type
            type_subst[expected] = e->args[i]->type;
        } else if (e->args[i]->type != TypeTable::Error && e->args[i]->type != expected) {
            ctx_.diag.error(call->args[i]->loc,
                std::string("argument type mismatch: expected ") +
                ctx_.types.name(expected) + ", got " +
                ctx_.types.name(e->args[i]->type));
        }
    }

    // For generic calls, substitute TypeVars in return type
    TypeId ret_type = sig.return_type;
    if (is_generic_call) {
        auto it2 = type_subst.find(ret_type);
        if (it2 != type_subst.end()) {
            ret_type = it2->second;
        }
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
        if (e->value->type != TypeTable::Error && !typesMatch(e->value->type, current_return_type_)) {
            ctx_.diag.error(expr->loc, std::string("return type mismatch: expected ") +
                        ctx_.types.name(current_return_type_) + ", got " +
                        ctx_.types.name(e->value->type));
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

    // Validate cast: integers can cast to other integers, ptrs to ints, ints to ptrs
    bool src_int = isIntegerType(src_type);
    bool dst_int = isIntegerType(target_type);
    bool src_ptr = false, dst_ptr = false;
    if (src_type < ctx_.types.size()) {
        auto k = ctx_.types.get(src_type).kind;
        src_ptr = (k == TypeKind::Ptr || k == TypeKind::PtrMut);
    }
    if (target_type < ctx_.types.size()) {
        auto k = ctx_.types.get(target_type).kind;
        dst_ptr = (k == TypeKind::Ptr || k == TypeKind::PtrMut);
    }

    bool valid = (src_int && dst_int) ||
                 (src_int && dst_ptr) ||
                 (src_ptr && dst_int) ||
                 (src_ptr && dst_ptr) ||
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
    // Loop type = inferred from break values or ctx_type
    if (ctx_type.has_value()) {
        e->type = *ctx_type;
    } else {
        e->type = TypeTable::Unit;
    }

    return e;
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

    // Determine element type from array type
    TypeId arr_type = e->array->type;
    if (arr_type != TypeTable::Error && arr_type < ctx_.types.size()) {
        const auto& ti = ctx_.types.get(arr_type);
        if (ti.kind == TypeKind::Array) {
            e->type = ti.array.element;
        } else {
            ctx_.diag.error(expr->loc, "index access on non-array type");
            e->type = TypeTable::Error;
        }
    } else {
        e->type = TypeTable::Error;
    }
    return e;
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
            } else if (!typesMatch(hir_body->type, arm_type)) {
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
                break;
            }
        }
    } else if (is_union && !has_catch_all) {
        for (uint32_t v = 0; v < scrut_info->union_.variant_count; ++v) {
            if (covered_variants.find(scrut_info->union_.variants[v].name) == covered_variants.end()) {
                ctx_.diag.error(matchE->loc,
                    std::string("non-exhaustive match on union '") +
                    std::string(scrut_info->union_.name) + "': missing variant '" +
                    std::string(scrut_info->union_.variants[v].name) + "'");
                break;
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
            TypeId expected = resolveType(decl->type);
            auto* s = ctx_.arena.make<HIRValDeclStmt>();
            s->kind = HIRStmt::Kind::ValDecl;
            s->loc = stmt->loc;
            s->name = ctx_.strings.intern(decl->name);
            s->type = expected;
            s->init = buildExpr(decl->init, expected);
            if (s->init->type != TypeTable::Error && s->init->type != expected) {
                ctx_.diag.error(stmt->loc, std::string("val type mismatch: expected ") +
                            ctx_.types.name(expected) + ", got " + ctx_.types.name(s->init->type));
            }
            local_vars_[decl->name] = expected;
            return s;
        }
        case Stmt::Kind::VarDecl: {
            auto* decl = static_cast<const VarDeclStmt*>(stmt);
            TypeId expected = resolveType(decl->type);
            auto* s = ctx_.arena.make<HIRVarDeclStmt>();
            s->kind = HIRStmt::Kind::VarDecl;
            s->loc = stmt->loc;
            s->name = ctx_.strings.intern(decl->name);
            s->type = expected;
            s->init = buildExpr(decl->init, expected);
            if (s->init->type != TypeTable::Error && s->init->type != expected) {
                ctx_.diag.error(stmt->loc, std::string("var type mismatch: expected ") +
                            ctx_.types.name(expected) + ", got " + ctx_.types.name(s->init->type));
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
                ctx_.diag.error(stmt->loc, std::string("assignment type mismatch: expected ") +
                            ctx_.types.name(it->second) + ", got " + ctx_.types.name(s->value->type));
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
    hfn->is_naked = false;
    hfn->is_interrupt = false;
    hfn->is_recursive = false;
    hfn->is_tail_recursive = false;
    hfn->purity = 0; // will be set by purity pass
    hfn->loc = expr->loc;
    hfn->type_param_count = 0;
    hfn->type_params = nullptr;

    // Save outer scope and set up lambda scope
    auto saved_locals = local_vars_;
    auto saved_mutables = mutable_vars_;
    auto saved_return = current_return_type_;
    local_vars_.clear();
    mutable_vars_.clear();

    for (uint32_t i = 0; i < lam->param_count; ++i) {
        auto param_name = ctx_.strings.intern(lam->params[i].name);
        hfn->params[i].name = param_name;
        hfn->params[i].type = param_types[i];
        hfn->params[i].loc = lam->params[i].loc;
        local_vars_[param_name] = param_types[i];
    }

    // Set return type for type checking inside the body
    current_return_type_ = return_type;

    // Build body
    hfn->body = buildExpr(lam->body, return_type != INVALID_TYPE ? std::optional<TypeId>(return_type) : std::nullopt);

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

    // Register the lifted function in fn_table_ so it can be found
    FnSig sig;
    sig.name = interned_name;
    sig.param_types = param_types;
    sig.return_type = return_type;
    fn_table_[interned_name] = sig;

    // Add to lifted lambdas list
    lifted_lambdas_.push_back(hfn);

    // Build the Fn type
    auto* tid_params = ctx_.arena.makeArray<TypeId>(param_types.size());
    for (size_t i = 0; i < param_types.size(); ++i) tid_params[i] = param_types[i];
    TypeId fn_type = ctx_.types.makeFn(
        std::span<const TypeId>(tid_params, param_types.size()), return_type);

    // Return an FnRef pointing to the lifted function
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
        }
        trait_table_[info.name] = info;
    }
}

void HIRBuilder::registerImpls(const Module* ast) {
    for (uint32_t i = 0; i < ast->impl_count; ++i) {
        auto* id = ast->impls[i];
        TypeId target_type = resolveType(id->target_type);

        // Get the type name for mangling
        std::string_view type_name = ctx_.types.name(target_type);

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
    std::string_view type_name = ctx_.types.name(type);
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

} // namespace kern
