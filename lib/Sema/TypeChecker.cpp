#include "kern/sema/TypeChecker.h"
#include <string>

namespace kern {

const char* typeName(Type t) {
    switch (t) {
        case Type::I8:     return "i8";
        case Type::I16:    return "i16";
        case Type::I32:    return "i32";
        case Type::I64:    return "i64";
        case Type::U8:     return "u8";
        case Type::U16:    return "u16";
        case Type::U32:    return "u32";
        case Type::U64:    return "u64";
        case Type::F32:    return "f32";
        case Type::F64:    return "f64";
        case Type::Bool:   return "bool";
        case Type::Unit:   return "Unit";
        case Type::Error:  return "<error>";
        case Type::Struct: return "struct";
        case Type::Enum:   return "enum";
        case Type::Union:  return "union";
        case Type::Ptr:    return "Ptr";
        case Type::PtrVar: return "Ptr<var>";
        case Type::String: return "String";
    }
    return "<unknown>";
}

bool isInteger(Type t) {
    switch (t) {
        case Type::I8: case Type::I16: case Type::I32: case Type::I64:
        case Type::U8: case Type::U16: case Type::U32: case Type::U64:
            return true;
        default:
            return false;
    }
}

bool isSigned(Type t) {
    switch (t) {
        case Type::I8: case Type::I16: case Type::I32: case Type::I64:
            return true;
        default:
            return false;
    }
}

bool isUnsigned(Type t) {
    switch (t) {
        case Type::U8: case Type::U16: case Type::U32: case Type::U64:
            return true;
        default:
            return false;
    }
}

bool isFloat(Type t) {
    return t == Type::F32 || t == Type::F64;
}

int bitWidth(Type t) {
    switch (t) {
        case Type::I8:  case Type::U8:  return 8;
        case Type::I16: case Type::U16: return 16;
        case Type::I32: case Type::U32: case Type::F32: return 32;
        case Type::I64: case Type::U64: case Type::F64: return 64;
        case Type::Bool: return 1;
        default: return 0;
    }
}

bool isStruct(Type t) { return t == Type::Struct; }
bool isEnum(Type t) { return t == Type::Enum; }
bool isUnion(Type t) { return t == Type::Union; }
bool isPtr(Type t) { return t == Type::Ptr || t == Type::PtrVar; }

int sizeBytes(Type t) {
    switch (t) {
        case Type::I8:  case Type::U8:  case Type::Bool: return 1;
        case Type::I16: case Type::U16: return 2;
        case Type::I32: case Type::U32: case Type::F32: return 4;
        case Type::I64: case Type::U64: case Type::F64: return 8;
        case Type::Ptr: case Type::PtrVar: return 8; // 64-bit pointers
        case Type::String: return 16; // fat pointer: ptr(8) + len(8)
        default: return 0;
    }
}

bool intFitsInType(int64_t value, Type t) {
    switch (t) {
        case Type::I8:  return value >= -128 && value <= 127;
        case Type::I16: return value >= -32768 && value <= 32767;
        case Type::I32: return value >= -2147483648LL && value <= 2147483647LL;
        case Type::I64: return true;
        case Type::U8:  return value >= 0 && value <= 255;
        case Type::U16: return value >= 0 && value <= 65535;
        case Type::U32: return value >= 0 && value <= 4294967295LL;
        case Type::U64: return value >= 0;
        default: return false;
    }
}

TypeChecker::TypeChecker(DiagnosticEngine& diag, Arena* arena) : diag_(diag), arena_(arena) {}

Type TypeChecker::resolveType(const TypeRef& ref) {
    if (ref.kind == TypeRef::Kind::Ptr) {
        // Ptr<T> or Ptr<var T>
        return ref.is_ptr_var ? Type::PtrVar : Type::Ptr;
    }
    if (ref.name == "i8")   return Type::I8;
    if (ref.name == "i16")  return Type::I16;
    if (ref.name == "i32")  return Type::I32;
    if (ref.name == "i64")  return Type::I64;
    if (ref.name == "u8")   return Type::U8;
    if (ref.name == "u16")  return Type::U16;
    if (ref.name == "u32")  return Type::U32;
    if (ref.name == "u64")  return Type::U64;
    if (ref.name == "f32")  return Type::F32;
    if (ref.name == "f64")  return Type::F64;
    if (ref.name == "bool") return Type::Bool;
    if (ref.name == "Unit") return Type::Unit;
    if (ref.name == "String") return Type::String;
    if (struct_defs_.count(ref.name)) return Type::Struct;
    if (enum_defs_.count(ref.name))  return Type::Enum;
    if (union_defs_.count(ref.name)) return Type::Union;
    diag_.error(ref.loc, std::string("unknown type '") + std::string(ref.name) + "'");
    return Type::Error;
}

Type TypeChecker::typeOfExpr(const Expr* expr) const {
    auto it = expr_types_.find(expr);
    if (it != expr_types_.end()) return it->second;
    return Type::Error;
}

const StructDef* TypeChecker::getStructDef(std::string_view name) const {
    auto it = struct_defs_.find(name);
    return (it != struct_defs_.end()) ? &it->second : nullptr;
}

std::string_view TypeChecker::structNameOfExpr(const Expr* expr) const {
    auto it = expr_struct_names_.find(expr);
    return (it != expr_struct_names_.end()) ? it->second : std::string_view{};
}

std::string_view TypeChecker::localStructName(std::string_view binding) const {
    auto it = local_struct_names_.find(binding);
    return (it != local_struct_names_.end()) ? it->second : std::string_view{};
}

const EnumDef* TypeChecker::getEnumDef(std::string_view name) const {
    auto it = enum_defs_.find(name);
    return (it != enum_defs_.end()) ? &it->second : nullptr;
}

const UnionDef* TypeChecker::getUnionDef(std::string_view name) const {
    auto it = union_defs_.find(name);
    return (it != union_defs_.end()) ? &it->second : nullptr;
}

std::string_view TypeChecker::enumNameOfExpr(const Expr* expr) const {
    auto it = expr_enum_names_.find(expr);
    return (it != expr_enum_names_.end()) ? it->second : std::string_view{};
}

std::string_view TypeChecker::unionNameOfExpr(const Expr* expr) const {
    auto it = expr_union_names_.find(expr);
    return (it != expr_union_names_.end()) ? it->second : std::string_view{};
}

std::string_view TypeChecker::localEnumName(std::string_view binding) const {
    auto it = local_enum_names_.find(binding);
    return (it != local_enum_names_.end()) ? it->second : std::string_view{};
}

std::string_view TypeChecker::localUnionName(std::string_view binding) const {
    auto it = local_union_names_.find(binding);
    return (it != local_union_names_.end()) ? it->second : std::string_view{};
}

Type TypeChecker::pointeeTypeOfExpr(const Expr* expr) const {
    auto it = expr_ptr_info_.find(expr);
    return (it != expr_ptr_info_.end()) ? it->second.pointee_type : Type::Error;
}

std::string_view TypeChecker::pointeeStructNameOfExpr(const Expr* expr) const {
    auto it = expr_ptr_info_.find(expr);
    return (it != expr_ptr_info_.end()) ? it->second.pointee_struct_name : std::string_view{};
}

Type TypeChecker::localPointeeType(std::string_view binding) const {
    auto it = local_ptr_info_.find(binding);
    return (it != local_ptr_info_.end()) ? it->second.pointee_type : Type::Error;
}

std::string_view TypeChecker::localPointeeStructName(std::string_view binding) const {
    auto it = local_ptr_info_.find(binding);
    return (it != local_ptr_info_.end()) ? it->second.pointee_struct_name : std::string_view{};
}

bool TypeChecker::check(Module* mod) {
    // Register struct definitions
    for (uint32_t i = 0; i < mod->struct_count; ++i) {
        auto* sd = mod->structs[i];
        StructDef def;
        def.name = sd->name;
        def.total_size = 0;
        def.alignment = 1;

        // Temporarily register so resolveType can find nested structs
        struct_defs_[sd->name] = def;
    }

    // Now compute layouts
    for (uint32_t i = 0; i < mod->struct_count; ++i) {
        auto* sd = mod->structs[i];
        StructDef def;
        def.name = sd->name;
        int32_t offset = 0;
        int32_t max_align = 1;

        for (uint32_t j = 0; j < sd->field_count; ++j) {
            auto& f = sd->fields[j];
            Type ft = resolveType(f.type);
            FieldDef fd;
            fd.name = f.name;
            fd.type = ft;
            fd.is_mutable = f.is_mutable;
            fd.struct_name = {};

            int field_size = 0;
            int field_align = 1;
            if (ft == Type::Struct) {
                fd.struct_name = f.type.name;
                auto nested_it = struct_defs_.find(f.type.name);
                if (nested_it != struct_defs_.end()) {
                    field_size = nested_it->second.total_size;
                    field_align = nested_it->second.alignment;
                }
            } else {
                field_size = sizeBytes(ft);
                field_align = field_size > 0 ? field_size : 1;
            }

            // Natural alignment
            if (field_align > 0) {
                offset = (offset + field_align - 1) / field_align * field_align;
            }
            fd.offset = offset;
            offset += field_size;
            if (field_align > max_align) max_align = field_align;

            def.fields.push_back(fd);
        }

        // Pad total size to alignment
        def.total_size = (offset + max_align - 1) / max_align * max_align;
        def.alignment = max_align;
        struct_defs_[sd->name] = def;
    }

    // Register enum definitions
    for (uint32_t i = 0; i < mod->enum_count; ++i) {
        auto* ed = mod->enums[i];
        EnumDef def;
        def.name = ed->name;
        def.tag_size = (ed->variant_count <= 256) ? 1 : (ed->variant_count <= 65536 ? 2 : 4);
        for (uint32_t j = 0; j < ed->variant_count; ++j) {
            EnumVariantDef vd;
            vd.name = ed->variants[j].name;
            vd.tag = static_cast<int32_t>(j);
            def.variants.push_back(vd);
        }
        enum_defs_[ed->name] = def;
    }

    // Register union definitions
    for (uint32_t i = 0; i < mod->union_count; ++i) {
        auto* ud = mod->unions[i];
        UnionDef def;
        def.name = ud->name;
        int32_t max_payload = 0;
        int32_t max_align = 1;

        for (uint32_t j = 0; j < ud->variant_count; ++j) {
            UnionVariantDef vd;
            vd.name = ud->variants[j].name;
            vd.tag = static_cast<int32_t>(j);
            vd.payload_size = 0;
            vd.payload_type = Type::Error;
            vd.payload_struct_name = {};

            if (ud->variants[j].payload_type) {
                Type pt = resolveType(*ud->variants[j].payload_type);
                vd.payload_type = pt;
                if (pt == Type::Struct) {
                    vd.payload_struct_name = ud->variants[j].payload_type->name;
                    auto sit = struct_defs_.find(vd.payload_struct_name);
                    if (sit != struct_defs_.end()) {
                        vd.payload_size = sit->second.total_size;
                        if (sit->second.alignment > max_align)
                            max_align = sit->second.alignment;
                    }
                } else {
                    vd.payload_size = sizeBytes(pt);
                    int pa = vd.payload_size > 0 ? vd.payload_size : 1;
                    if (pa > max_align) max_align = pa;
                }
            }
            if (vd.payload_size > max_payload) max_payload = vd.payload_size;
            def.variants.push_back(vd);
        }

        def.tag_size = (ud->variant_count <= 256) ? 1 : (ud->variant_count <= 65536 ? 2 : 4);
        def.max_payload_size = max_payload;
        // Pad tag to alignment boundary for payload
        int32_t data_offset = def.tag_size;
        if (max_align > 1 && max_payload > 0) {
            data_offset = (data_offset + max_align - 1) / max_align * max_align;
        }
        def.total_size = data_offset + max_payload;
        // Pad total to max alignment
        if (max_align > 1) {
            def.total_size = (def.total_size + max_align - 1) / max_align * max_align;
        }
        def.alignment = max_align;
        union_defs_[ud->name] = def;
    }

    // First pass: register all function signatures
    for (uint32_t i = 0; i < mod->fn_count; ++i) {
        FnDecl* fn = mod->functions[i];
        FnSig sig;
        sig.name = fn->name;
        sig.return_type = resolveType(fn->return_type);
        if (sig.return_type == Type::Struct || sig.return_type == Type::Enum ||
            sig.return_type == Type::Union) {
            sig.return_struct_name = fn->return_type.name;
        }
        for (uint32_t j = 0; j < fn->param_count; ++j) {
            Type pt = resolveType(fn->params[j].type);
            sig.param_types.push_back(pt);
            std::string_view pname = {};
            if (pt == Type::Struct || pt == Type::Enum || pt == Type::Union) {
                pname = fn->params[j].type.name;
            }
            sig.param_struct_names.push_back(pname);
        }
        fn_table_[fn->name] = sig;
    }

    // Validate parameter count limit (System V ABI: 6 integer regs)
    for (uint32_t i = 0; i < mod->fn_count; ++i) {
        if (mod->functions[i]->param_count > 6) {
            diag_.error(mod->functions[i]->loc,
                std::string("function '") + std::string(mod->functions[i]->name) +
                "' has " + std::to_string(mod->functions[i]->param_count) +
                " parameters, maximum is 6 (System V ABI register limit)");
        }
    }

    // Second pass: type-check function bodies
    for (uint32_t i = 0; i < mod->fn_count; ++i) {
        checkFn(mod->functions[i]);
    }

    return !diag_.hasErrors();
}

Type TypeChecker::checkFn(FnDecl* fn) {
    local_vars_.clear();
    mutable_vars_.clear();
    local_struct_names_.clear();
    local_enum_names_.clear();
    local_union_names_.clear();
    local_ptr_info_.clear();
    current_return_type_ = resolveType(fn->return_type);

    // Intrinsic functions: signature only, no body to check
    if (fn->is_intrinsic) {
        return current_return_type_;
    }

    // Register parameters
    for (uint32_t i = 0; i < fn->param_count; ++i) {
        Type pt = resolveType(fn->params[i].type);
        local_vars_[fn->params[i].name] = pt;
        if (pt == Type::Struct) {
            local_struct_names_[fn->params[i].name] = fn->params[i].type.name;
        } else if (pt == Type::Enum) {
            local_enum_names_[fn->params[i].name] = fn->params[i].type.name;
        } else if (pt == Type::Union) {
            local_union_names_[fn->params[i].name] = fn->params[i].type.name;
        } else if (isPtr(pt)) {
            // Register pointer pointee info
            auto& pref = fn->params[i].type;
            if (pref.pointee) {
                Type pointee_type = resolveType(*pref.pointee);
                PtrInfo info{pointee_type, {}};
                if (pointee_type == Type::Struct) {
                    info.pointee_struct_name = pref.pointee->name;
                } else if (pointee_type == Type::Union) {
                    info.pointee_struct_name = pref.pointee->name;
                }
                local_ptr_info_[fn->params[i].name] = info;
            }
        }
    }

    Type body_type = checkExpr(fn->body, current_return_type_);

    if (body_type != Type::Error && body_type != current_return_type_) {
        diag_.error(fn->loc, std::string("function '") + std::string(fn->name) +
                    "' declared to return " + typeName(current_return_type_) +
                    " but body has type " + typeName(body_type));
        diag_.note(fn->return_type.loc, std::string("return type '") +
                   typeName(current_return_type_) + "' declared here");
    }

    return body_type;
}

Type TypeChecker::checkExpr(Expr* expr, std::optional<Type> ctx) {
    if (!expr) return Type::Unit;

    Type result = Type::Error;

    switch (expr->kind) {
        case Expr::Kind::IntLit: {
            auto* lit = static_cast<IntLitExpr*>(expr);
            if (ctx && isInteger(*ctx)) {
                if (intFitsInType(lit->value, *ctx)) {
                    result = *ctx;
                } else {
                    diag_.error(expr->loc,
                        std::string("integer literal ") + std::to_string(lit->value) +
                        " is out of range for type " + typeName(*ctx));
                    result = Type::Error;
                }
            } else {
                result = Type::I64;
            }
            break;
        }

        case Expr::Kind::FloatLit: {
            auto* fl = static_cast<FloatLitExpr*>(expr);
            if (ctx && isFloat(*ctx)) {
                result = *ctx;
            } else {
                result = fl->is_f32 ? Type::F32 : Type::F64;
            }
            break;
        }

        case Expr::Kind::BoolLit:
            result = Type::Bool;
            break;

        case Expr::Kind::StringLit:
            result = Type::String;
            break;

        case Expr::Kind::Ident: {
            auto* ident = static_cast<IdentExpr*>(expr);
            auto it = local_vars_.find(ident->name);
            if (it == local_vars_.end()) {
                diag_.error(expr->loc, std::string("undeclared identifier '") +
                            std::string(ident->name) + "'");
                result = Type::Error;
            } else {
                result = it->second;
                if (result == Type::Struct) {
                    auto sn = localStructName(ident->name);
                    if (!sn.empty()) expr_struct_names_[expr] = sn;
                } else if (result == Type::Enum) {
                    auto en = localEnumName(ident->name);
                    if (!en.empty()) expr_enum_names_[expr] = en;
                } else if (result == Type::Union) {
                    auto un = localUnionName(ident->name);
                    if (!un.empty()) expr_union_names_[expr] = un;
                }
            }
            break;
        }

        case Expr::Kind::BinOp: {
            auto* bin = static_cast<BinOpExpr*>(expr);
            // Propagate context to operands for arith/comparison ops
            std::optional<Type> operand_ctx = std::nullopt;
            bool is_cmp = false;
            switch (bin->op) {
                case BinOpKind::Add: case BinOpKind::Sub:
                case BinOpKind::Mul: case BinOpKind::Div:
                    operand_ctx = ctx;
                    break;
                case BinOpKind::Eq:  case BinOpKind::NotEq:
                case BinOpKind::Lt:  case BinOpKind::LtEq:
                case BinOpKind::Gt:  case BinOpKind::GtEq:
                    is_cmp = true;
                    break;
                case BinOpKind::And: case BinOpKind::Or:
                    break;
            }
            Type lhs = checkExpr(bin->lhs, operand_ctx);
            // For comparisons: use lhs type as context for rhs (enables literal coercion)
            Type rhs = checkExpr(bin->rhs, is_cmp ? std::optional<Type>(lhs) : operand_ctx);
            if (lhs == Type::Error || rhs == Type::Error) {
                result = Type::Error;
                break;
            }

            switch (bin->op) {
                case BinOpKind::Add:
                case BinOpKind::Sub:
                case BinOpKind::Mul:
                case BinOpKind::Div:
                    if (!(isInteger(lhs) || isFloat(lhs)) || lhs != rhs) {
                        diag_.error(expr->loc,
                            std::string("arithmetic operators require same numeric type operands, got ") +
                            typeName(lhs) + " and " + typeName(rhs));
                        result = Type::Error;
                    } else {
                        result = lhs;
                    }
                    break;

                case BinOpKind::Eq:
                case BinOpKind::NotEq:
                case BinOpKind::Lt:
                case BinOpKind::LtEq:
                case BinOpKind::Gt:
                case BinOpKind::GtEq:
                    if (lhs != rhs) {
                        diag_.error(expr->loc, "comparison operators require same-type operands");
                        result = Type::Error;
                    } else {
                        result = Type::Bool;
                    }
                    break;

                case BinOpKind::And:
                case BinOpKind::Or:
                    if (lhs != Type::Bool || rhs != Type::Bool) {
                        diag_.error(expr->loc, "'and'/'or' require bool operands");
                        result = Type::Error;
                    } else {
                        result = Type::Bool;
                    }
                    break;
            }
            break;
        }

        case Expr::Kind::UnaryOp: {
            auto* unary = static_cast<UnaryOpExpr*>(expr);
            Type operand = checkExpr(unary->operand,
                unary->op == UnaryOpKind_t::Neg ? ctx : std::nullopt);
            if (operand == Type::Error) {
                result = Type::Error;
                break;
            }

            switch (unary->op) {
                case UnaryOpKind_t::Neg:
                    if (!isSigned(operand) && !isFloat(operand)) {
                        diag_.error(expr->loc,
                            std::string("unary '-' requires signed integer or float operand, got ") +
                            typeName(operand));
                        result = Type::Error;
                    } else {
                        result = operand;
                    }
                    break;
                case UnaryOpKind_t::Not:
                    if (operand != Type::Bool) {
                        diag_.error(expr->loc, "'not' requires bool operand");
                        result = Type::Error;
                    } else {
                        result = Type::Bool;
                    }
                    break;
                case UnaryOpKind_t::Deref: {
                    if (!isPtr(operand)) {
                        diag_.error(expr->loc,
                            std::string("cannot dereference non-pointer type '") +
                            typeName(operand) + "'");
                        result = Type::Error;
                    } else {
                        // Get the pointee type
                        Type pointee = pointeeTypeOfExpr(unary->operand);
                        if (pointee == Type::Error) {
                            // Try local binding
                            if (unary->operand->kind == Expr::Kind::Ident) {
                                auto* ident = static_cast<IdentExpr*>(unary->operand);
                                pointee = localPointeeType(ident->name);
                            }
                        }
                        result = pointee;
                        if (result == Type::Struct) {
                            // Propagate struct name
                            auto sn = pointeeStructNameOfExpr(unary->operand);
                            if (sn.empty() && unary->operand->kind == Expr::Kind::Ident) {
                                sn = localPointeeStructName(
                                    static_cast<IdentExpr*>(unary->operand)->name);
                            }
                            if (!sn.empty()) {
                                expr_struct_names_[expr] = sn;
                            }
                        } else if (result == Type::Union) {
                            auto sn = pointeeStructNameOfExpr(unary->operand);
                            if (sn.empty() && unary->operand->kind == Expr::Kind::Ident) {
                                sn = localPointeeStructName(
                                    static_cast<IdentExpr*>(unary->operand)->name);
                            }
                            if (!sn.empty()) {
                                expr_union_names_[expr] = sn;
                            }
                        } else if (result == Type::Enum) {
                            auto sn = pointeeStructNameOfExpr(unary->operand);
                            if (sn.empty() && unary->operand->kind == Expr::Kind::Ident) {
                                sn = localPointeeStructName(
                                    static_cast<IdentExpr*>(unary->operand)->name);
                            }
                            if (!sn.empty()) {
                                expr_enum_names_[expr] = sn;
                            }
                        }
                    }
                    break;
                }
                case UnaryOpKind_t::AddrOf: {
                    // &x → Ptr<T>
                    result = Type::Ptr;
                    PtrInfo info{operand, {}};
                    if (operand == Type::Struct) {
                        info.pointee_struct_name = structNameOfExpr(unary->operand);
                        if (info.pointee_struct_name.empty() &&
                            unary->operand->kind == Expr::Kind::Ident) {
                            info.pointee_struct_name = localStructName(
                                static_cast<IdentExpr*>(unary->operand)->name);
                        }
                    } else if (operand == Type::Union) {
                        info.pointee_struct_name = unionNameOfExpr(unary->operand);
                        if (info.pointee_struct_name.empty() &&
                            unary->operand->kind == Expr::Kind::Ident) {
                            info.pointee_struct_name = localUnionName(
                                static_cast<IdentExpr*>(unary->operand)->name);
                        }
                    }
                    expr_ptr_info_[expr] = info;
                    break;
                }
                case UnaryOpKind_t::AddrOfVar: {
                    // &var x → Ptr<var T> — operand must be a mutable variable
                    if (unary->operand->kind != Expr::Kind::Ident) {
                        diag_.error(expr->loc, "'&var' requires a variable name");
                        result = Type::Error;
                        break;
                    }
                    auto* ident = static_cast<IdentExpr*>(unary->operand);
                    if (mutable_vars_.find(ident->name) == mutable_vars_.end()) {
                        diag_.error(expr->loc,
                            std::string("'&var' requires a 'var' binding, but '") +
                            std::string(ident->name) + "' is immutable");
                        result = Type::Error;
                        break;
                    }
                    result = Type::PtrVar;
                    PtrInfo info{operand, {}};
                    if (operand == Type::Struct) {
                        info.pointee_struct_name = localStructName(ident->name);
                    } else if (operand == Type::Union) {
                        info.pointee_struct_name = localUnionName(ident->name);
                    }
                    expr_ptr_info_[expr] = info;
                    break;
                }
            }
            break;
        }

        case Expr::Kind::Call: {
            auto* call = static_cast<CallExpr*>(expr);
            auto it = fn_table_.find(call->callee);
            if (it == fn_table_.end()) {
                diag_.error(expr->loc, std::string("undeclared function '") +
                            std::string(call->callee) + "'");
                result = Type::Error;
                break;
            }
            const FnSig& sig = it->second;
            if (call->arg_count != sig.param_types.size()) {
                diag_.error(expr->loc, std::string("function '") + std::string(call->callee) +
                            "' expects " + std::to_string(sig.param_types.size()) +
                            " arguments, got " + std::to_string(call->arg_count));
                result = Type::Error;
                break;
            }
            for (uint32_t i = 0; i < call->arg_count; ++i) {
                Type arg = checkExpr(call->args[i], sig.param_types[i]);
                if (arg != Type::Error && arg != sig.param_types[i]) {
                    diag_.error(call->args[i]->loc,
                                std::string("argument type mismatch: expected ") +
                                typeName(sig.param_types[i]) + ", got " + typeName(arg));
                }
            }
            result = sig.return_type;
            if (result == Type::Struct && !sig.return_struct_name.empty()) {
                expr_struct_names_[expr] = sig.return_struct_name;
            }
            break;
        }

        case Expr::Kind::If: {
            auto* ifE = static_cast<IfExpr*>(expr);
            Type cond = checkExpr(ifE->condition);
            if (cond != Type::Error && cond != Type::Bool) {
                diag_.error(ifE->condition->loc, "if condition must be bool");
            }
            Type then_t = checkExpr(ifE->then_branch, ctx);
            if (ifE->else_branch) {
                Type else_t = checkExpr(ifE->else_branch, ctx);
                if (then_t != Type::Error && else_t != Type::Error && then_t != else_t) {
                    diag_.error(expr->loc, std::string("if branches have different types: ") +
                                typeName(then_t) + " vs " + typeName(else_t));
                    result = Type::Error;
                } else {
                    result = then_t;
                }
            } else {
                result = then_t;
            }
            break;
        }

        case Expr::Kind::Block:
            result = checkBlock(static_cast<BlockExpr*>(expr), ctx);
            break;

        case Expr::Kind::Return: {
            auto* ret = static_cast<ReturnExpr*>(expr);
            if (ret->value) {
                Type val = checkExpr(ret->value, current_return_type_);
                if (val != Type::Error && val != current_return_type_) {
                    diag_.error(expr->loc, std::string("return type mismatch: expected ") +
                                typeName(current_return_type_) + ", got " + typeName(val));
                }
                result = val;
            } else {
                result = Type::Unit;
            }
            break;
        }

        case Expr::Kind::StructLit: {
            auto* sl = static_cast<StructLitExpr*>(expr);
            auto def_it = struct_defs_.find(sl->struct_name);
            if (def_it == struct_defs_.end()) {
                diag_.error(expr->loc, std::string("unknown struct '") +
                            std::string(sl->struct_name) + "'");
                result = Type::Error;
                break;
            }
            const auto& def = def_it->second;

            // Check field count
            if (sl->field_count != def.fields.size()) {
                diag_.error(expr->loc, std::string("struct '") + std::string(sl->struct_name) +
                            "' expects " + std::to_string(def.fields.size()) +
                            " fields, got " + std::to_string(sl->field_count));
                result = Type::Error;
                break;
            }

            // Check each field by name
            bool has_error = false;
            for (uint32_t i = 0; i < sl->field_count; ++i) {
                bool found = false;
                for (const auto& fd : def.fields) {
                    if (fd.name == sl->fields[i].name) {
                        found = true;
                        std::optional<Type> field_ctx = fd.type;
                        Type actual = checkExpr(sl->fields[i].value, field_ctx);
                        if (actual == Type::Struct && fd.type == Type::Struct) {
                            expr_struct_names_[sl->fields[i].value] = fd.struct_name;
                        } else if (actual != Type::Error && actual != fd.type) {
                            diag_.error(sl->fields[i].loc,
                                std::string("field '") + std::string(fd.name) +
                                "' expects " + typeName(fd.type) +
                                ", got " + typeName(actual));
                            has_error = true;
                        }
                        break;
                    }
                }
                if (!found) {
                    diag_.error(sl->fields[i].loc,
                        std::string("struct '") + std::string(sl->struct_name) +
                        "' has no field named '" + std::string(sl->fields[i].name) + "'");
                    has_error = true;
                }
            }

            if (has_error) {
                result = Type::Error;
            } else {
                result = Type::Struct;
                expr_struct_names_[expr] = sl->struct_name;
            }
            break;
        }

        case Expr::Kind::FieldAccess: {
            auto* fa = static_cast<FieldAccessExpr*>(expr);
            Type obj_type = checkExpr(fa->object);

            // String field access: .data and .len
            if (obj_type == Type::String) {
                if (fa->field_name == "len") {
                    result = Type::U64;
                } else if (fa->field_name == "data") {
                    result = Type::Ptr;
                    expr_ptr_info_[expr] = {Type::U8, ""};
                } else {
                    diag_.error(expr->loc, std::string("String has no field named '") +
                                std::string(fa->field_name) + "'");
                    result = Type::Error;
                }
                break;
            }

            if (obj_type != Type::Struct) {
                if (obj_type != Type::Error) {
                    diag_.error(expr->loc, std::string("field access requires struct type, got ") +
                                typeName(obj_type));
                }
                result = Type::Error;
                break;
            }

            // Find struct name from object
            std::string_view sname;
            auto eit = expr_struct_names_.find(fa->object);
            if (eit != expr_struct_names_.end()) {
                sname = eit->second;
            } else if (fa->object->kind == Expr::Kind::Ident) {
                auto* ident = static_cast<IdentExpr*>(fa->object);
                sname = localStructName(ident->name);
            }

            if (sname.empty()) {
                diag_.error(expr->loc, "cannot determine struct type for field access");
                result = Type::Error;
                break;
            }

            auto def_it = struct_defs_.find(sname);
            if (def_it == struct_defs_.end()) {
                result = Type::Error;
                break;
            }

            bool found = false;
            for (const auto& fd : def_it->second.fields) {
                if (fd.name == fa->field_name) {
                    result = fd.type;
                    if (fd.type == Type::Struct) {
                        expr_struct_names_[expr] = fd.struct_name;
                    }
                    found = true;
                    break;
                }
            }
            if (!found) {
                diag_.error(expr->loc, std::string("struct '") + std::string(sname) +
                            "' has no field named '" + std::string(fa->field_name) + "'");
                result = Type::Error;
            }
            break;
        }

        case Expr::Kind::EnumAccess: {
            auto* ea = static_cast<EnumAccessExpr*>(expr);
            auto it = enum_defs_.find(ea->enum_name);
            if (it == enum_defs_.end()) {
                diag_.error(expr->loc, std::string("unknown enum '") +
                            std::string(ea->enum_name) + "'");
                result = Type::Error;
                break;
            }
            bool found = false;
            for (const auto& v : it->second.variants) {
                if (v.name == ea->variant_name) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                diag_.error(expr->loc, std::string("enum '") + std::string(ea->enum_name) +
                            "' has no variant '" + std::string(ea->variant_name) + "'");
                result = Type::Error;
                break;
            }
            result = Type::Enum;
            expr_enum_names_[expr] = ea->enum_name;
            break;
        }

        case Expr::Kind::UnionVariant: {
            auto* uv = static_cast<UnionVariantExpr*>(expr);
            auto it = union_defs_.find(uv->union_name);
            if (it == union_defs_.end()) {
                diag_.error(expr->loc, std::string("unknown union '") +
                            std::string(uv->union_name) + "'");
                result = Type::Error;
                break;
            }
            const UnionVariantDef* vdef = nullptr;
            for (const auto& v : it->second.variants) {
                if (v.name == uv->variant_name) {
                    vdef = &v;
                    break;
                }
            }
            if (!vdef) {
                diag_.error(expr->loc, std::string("union '") + std::string(uv->union_name) +
                            "' has no variant '" + std::string(uv->variant_name) + "'");
                result = Type::Error;
                break;
            }
            // Check payload
            if (vdef->payload_type == Type::Error && uv->payload != nullptr) {
                diag_.error(expr->loc, std::string("variant '") + std::string(uv->variant_name) +
                            "' takes no payload");
                result = Type::Error;
                break;
            }
            if (vdef->payload_type != Type::Error && uv->payload == nullptr) {
                diag_.error(expr->loc, std::string("variant '") + std::string(uv->variant_name) +
                            "' requires a payload of type " + typeName(vdef->payload_type));
                result = Type::Error;
                break;
            }
            if (uv->payload) {
                std::optional<Type> payload_ctx;
                if (vdef->payload_type != Type::Error) payload_ctx = vdef->payload_type;
                Type payload_type = checkExpr(uv->payload, payload_ctx);
                if (payload_type != Type::Error && payload_type != vdef->payload_type) {
                    diag_.error(uv->payload->loc,
                        std::string("variant '") + std::string(uv->variant_name) +
                        "' expects payload of type " + typeName(vdef->payload_type) +
                        ", got " + typeName(payload_type));
                    result = Type::Error;
                    break;
                }
            }
            result = Type::Union;
            expr_union_names_[expr] = uv->union_name;
            break;
        }

        case Expr::Kind::Match: {
            auto* matchE = static_cast<MatchExpr*>(expr);
            Type scrut_type = checkExpr(matchE->scrutinee);
            if (scrut_type == Type::Error) {
                result = Type::Error;
                break;
            }

            // Save/restore local vars for arm scoping
            auto saved_locals = local_vars_;

            // Resolve enum/union name for scrutinee
            std::string_view scrut_enum_name;
            std::string_view scrut_union_name;
            const EnumDef* scrut_enum = nullptr;
            const UnionDef* scrut_union = nullptr;
            if (scrut_type == Type::Enum) {
                scrut_enum_name = enumNameOfExpr(matchE->scrutinee);
                if (scrut_enum_name.empty()) {
                    // Try from ident
                    if (matchE->scrutinee->kind == Expr::Kind::Ident) {
                        auto* ident = static_cast<IdentExpr*>(matchE->scrutinee);
                        scrut_enum_name = localEnumName(ident->name);
                    }
                }
                if (!scrut_enum_name.empty()) {
                    scrut_enum = getEnumDef(scrut_enum_name);
                }
            } else if (scrut_type == Type::Union) {
                scrut_union_name = unionNameOfExpr(matchE->scrutinee);
                if (scrut_union_name.empty()) {
                    if (matchE->scrutinee->kind == Expr::Kind::Ident) {
                        auto* ident = static_cast<IdentExpr*>(matchE->scrutinee);
                        scrut_union_name = localUnionName(ident->name);
                    }
                }
                if (!scrut_union_name.empty()) {
                    scrut_union = getUnionDef(scrut_union_name);
                }
            }

            Type arm_type = Type::Error;
            bool has_catch_all = false;
            bool has_true = false, has_false = false;
            std::unordered_set<std::string_view> covered_variants;

            for (uint32_t i = 0; i < matchE->arm_count; ++i) {
                auto& arm = matchE->arms[i];
                local_vars_ = saved_locals;

                // Check pattern compatibility with scrutinee
                switch (arm.pattern->kind) {
                    case Pattern::Kind::IntLit:
                        if (!isInteger(scrut_type)) {
                            diag_.error(arm.pattern->loc,
                                std::string("integer pattern incompatible with scrutinee type ") +
                                typeName(scrut_type));
                        }
                        break;
                    case Pattern::Kind::BoolLit:
                        if (scrut_type != Type::Bool) {
                            diag_.error(arm.pattern->loc,
                                "bool pattern incompatible with non-bool scrutinee");
                        } else {
                            auto* bp = static_cast<BoolLitPattern*>(arm.pattern);
                            if (bp->value) has_true = true; else has_false = true;
                        }
                        break;
                    case Pattern::Kind::Variable: {
                        auto* vp = static_cast<VariablePattern*>(arm.pattern);
                        // Check if this is actually an enum variant name or union variant name
                        if (scrut_enum) {
                            bool is_variant = false;
                            for (const auto& v : scrut_enum->variants) {
                                if (v.name == vp->name) {
                                    is_variant = true;
                                    covered_variants.insert(v.name);
                                    // Promote to EnumPattern at semantic level
                                    arm.pattern->kind = Pattern::Kind::Enum;
                                    break;
                                }
                            }
                            if (!is_variant) {
                                // It's a catch-all variable binding
                                local_vars_[vp->name] = scrut_type;
                                local_enum_names_[vp->name] = scrut_enum_name;
                                if (!arm.guard) has_catch_all = true;
                            }
                        } else if (scrut_union) {
                            bool is_variant = false;
                            for (const auto& v : scrut_union->variants) {
                                if (v.name == vp->name) {
                                    is_variant = true;
                                    covered_variants.insert(v.name);
                                    // Promote to Union pattern — allocate proper node
                                    if (arena_) {
                                        auto* up = arena_->make<UnionPattern>();
                                        up->kind = Pattern::Kind::Union;
                                        up->loc = vp->loc;
                                        up->variant_name = vp->name;
                                        up->inner = nullptr;
                                        up->field_bindings = nullptr;
                                        up->field_binding_count = 0;
                                        arm.pattern = up;
                                    } else {
                                        arm.pattern->kind = Pattern::Kind::Union;
                                    }
                                    break;
                                }
                            }
                            if (!is_variant) {
                                local_vars_[vp->name] = scrut_type;
                                local_union_names_[vp->name] = scrut_union_name;
                                if (!arm.guard) has_catch_all = true;
                            }
                        } else {
                            local_vars_[vp->name] = scrut_type;
                            if (!arm.guard) has_catch_all = true;
                        }
                        break;
                    }
                    case Pattern::Kind::Wildcard:
                        if (!arm.guard) has_catch_all = true;
                        break;
                    case Pattern::Kind::Enum: {
                        auto* ep = static_cast<EnumPattern*>(arm.pattern);
                        if (scrut_enum) {
                            covered_variants.insert(ep->variant_name);
                        }
                        break;
                    }
                    case Pattern::Kind::Union: {
                        auto* up = static_cast<UnionPattern*>(arm.pattern);
                        if (scrut_union) {
                            covered_variants.insert(up->variant_name);
                            // Find variant def to bind inner pattern
                            for (const auto& v : scrut_union->variants) {
                                if (v.name == up->variant_name) {
                                    if (up->inner && v.payload_type != Type::Error) {
                                        // Bind inner variable to payload type
                                        if (up->inner->kind == Pattern::Kind::Variable) {
                                            auto* inner_vp = static_cast<VariablePattern*>(up->inner);
                                            local_vars_[inner_vp->name] = v.payload_type;
                                            if (v.payload_type == Type::Struct) {
                                                local_struct_names_[inner_vp->name] = v.payload_struct_name;
                                            }
                                        }
                                    }
                                    if (up->field_binding_count > 0 && v.payload_type == Type::Struct) {
                                        // Struct destructuring: bind each field
                                        auto sdef = getStructDef(v.payload_struct_name);
                                        if (sdef) {
                                            for (uint32_t fb = 0; fb < up->field_binding_count; ++fb) {
                                                auto& binding = up->field_bindings[fb];
                                                for (const auto& fd : sdef->fields) {
                                                    if (fd.name == binding.field_name) {
                                                        local_vars_[binding.binding_name] = fd.type;
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

                // Check guard
                if (arm.guard) {
                    Type guard_type = checkExpr(arm.guard);
                    if (guard_type != Type::Error && guard_type != Type::Bool) {
                        diag_.error(arm.guard->loc, "match guard must be bool");
                    }
                }

                // Check body
                Type body_type = checkExpr(arm.body, ctx);
                if (body_type == Type::Error) continue;

                if (arm_type == Type::Error) {
                    arm_type = body_type;
                } else if (body_type != arm_type) {
                    diag_.error(arm.body->loc,
                        std::string("match arm type mismatch: expected ") +
                        typeName(arm_type) + ", got " + typeName(body_type));
                }
            }

            // Restore locals
            local_vars_ = saved_locals;

            // Exhaustiveness check
            if (scrut_type == Type::Bool) {
                if (!has_catch_all && !(has_true && has_false)) {
                    diag_.error(matchE->loc, "non-exhaustive match on bool: missing patterns");
                }
            } else if (isInteger(scrut_type)) {
                if (!has_catch_all) {
                    diag_.error(matchE->loc,
                        "non-exhaustive match on integer: add a wildcard '_' or variable pattern");
                }
            } else if (scrut_enum && !has_catch_all) {
                for (const auto& v : scrut_enum->variants) {
                    if (covered_variants.find(v.name) == covered_variants.end()) {
                        diag_.error(matchE->loc,
                            std::string("non-exhaustive match on enum '") +
                            std::string(scrut_enum_name) + "': missing variant '" +
                            std::string(v.name) + "'");
                        break;
                    }
                }
            } else if (scrut_union && !has_catch_all) {
                for (const auto& v : scrut_union->variants) {
                    if (covered_variants.find(v.name) == covered_variants.end()) {
                        diag_.error(matchE->loc,
                            std::string("non-exhaustive match on union '") +
                            std::string(scrut_union_name) + "': missing variant '" +
                            std::string(v.name) + "'");
                        break;
                    }
                }
            }

            result = arm_type;
            break;
        }
    }

    expr_types_[expr] = result;
    return result;
}

Type TypeChecker::checkBlock(BlockExpr* block, std::optional<Type> ctx) {
    for (uint32_t i = 0; i < block->stmt_count; ++i) {
        checkStmt(block->stmts[i]);
    }
    if (block->result) {
        return checkExpr(block->result, ctx);
    }
    return Type::Unit;
}

void TypeChecker::checkStmt(Stmt* stmt) {
    switch (stmt->kind) {
        case Stmt::Kind::ValDecl: {
            auto* decl = static_cast<ValDeclStmt*>(stmt);
            Type expected = resolveType(decl->type);
            Type actual = checkExpr(decl->init, expected);
            if (actual != Type::Error && actual != expected) {
                // For structs, check struct names match
                if (expected == Type::Struct && actual == Type::Struct) {
                    auto init_sname = structNameOfExpr(decl->init);
                    if (!init_sname.empty() && init_sname != decl->type.name) {
                        diag_.error(stmt->loc, std::string("val type mismatch: expected struct ") +
                                    std::string(decl->type.name) + ", got struct " +
                                    std::string(init_sname));
                    }
                } else {
                    diag_.error(stmt->loc, std::string("val type mismatch: expected ") +
                                typeName(expected) + ", got " + typeName(actual));
                }
            }
            local_vars_[decl->name] = expected;
            if (expected == Type::Struct) {
                local_struct_names_[decl->name] = decl->type.name;
            } else if (expected == Type::Enum) {
                local_enum_names_[decl->name] = decl->type.name;
            } else if (expected == Type::Union) {
                local_union_names_[decl->name] = decl->type.name;
            } else if (isPtr(expected) && decl->type.pointee) {
                Type pointee_type = resolveType(*decl->type.pointee);
                PtrInfo info{pointee_type, {}};
                if (pointee_type == Type::Struct) info.pointee_struct_name = decl->type.pointee->name;
                else if (pointee_type == Type::Union) info.pointee_struct_name = decl->type.pointee->name;
                local_ptr_info_[decl->name] = info;
            }
            break;
        }
        case Stmt::Kind::VarDecl: {
            auto* decl = static_cast<VarDeclStmt*>(stmt);
            Type expected = resolveType(decl->type);
            Type actual = checkExpr(decl->init, expected);
            if (actual != Type::Error && actual != expected) {
                if (expected == Type::Struct && actual == Type::Struct) {
                    auto init_sname = structNameOfExpr(decl->init);
                    if (!init_sname.empty() && init_sname != decl->type.name) {
                        diag_.error(stmt->loc, std::string("var type mismatch: expected struct ") +
                                    std::string(decl->type.name) + ", got struct " +
                                    std::string(init_sname));
                    }
                } else {
                    diag_.error(stmt->loc, std::string("var type mismatch: expected ") +
                                typeName(expected) + ", got " + typeName(actual));
                }
            }
            local_vars_[decl->name] = expected;
            mutable_vars_.insert(decl->name);
            if (expected == Type::Struct) {
                local_struct_names_[decl->name] = decl->type.name;
            } else if (expected == Type::Enum) {
                local_enum_names_[decl->name] = decl->type.name;
            } else if (expected == Type::Union) {
                local_union_names_[decl->name] = decl->type.name;
            } else if (isPtr(expected) && decl->type.pointee) {
                Type pointee_type = resolveType(*decl->type.pointee);
                PtrInfo info{pointee_type, {}};
                if (pointee_type == Type::Struct) info.pointee_struct_name = decl->type.pointee->name;
                else if (pointee_type == Type::Union) info.pointee_struct_name = decl->type.pointee->name;
                local_ptr_info_[decl->name] = info;
            }
            break;
        }
        case Stmt::Kind::Assign: {
            auto* assign = static_cast<AssignStmt*>(stmt);
            auto it = local_vars_.find(assign->name);
            if (it == local_vars_.end()) {
                diag_.error(stmt->loc, std::string("undeclared identifier '") +
                            std::string(assign->name) + "'");
                break;
            }
            if (mutable_vars_.find(assign->name) == mutable_vars_.end()) {
                diag_.error(stmt->loc, std::string("cannot assign to immutable binding '") +
                            std::string(assign->name) + "'; use 'var' instead of 'val'");
                diag_.note(stmt->loc, "to make this binding mutable, declare it with 'var'");
                break;
            }
            Type actual = checkExpr(assign->value, it->second);
            if (actual != Type::Error && actual != it->second) {
                diag_.error(stmt->loc, std::string("assignment type mismatch: expected ") +
                            typeName(it->second) + ", got " + typeName(actual));
            }
            break;
        }
        case Stmt::Kind::ExprStmt:
            checkExpr(static_cast<ExprStmt*>(stmt)->expr);
            break;
        case Stmt::Kind::FieldAssign: {
            auto* fas = static_cast<FieldAssignStmt*>(stmt);
            // Check target — must resolve to a struct field
            Type target_type = checkExpr(fas->target);

            // Walk the chain to find the root binding and check mutability
            // Root must be var binding
            Expr* root = fas->target;
            while (root->kind == Expr::Kind::FieldAccess) {
                root = static_cast<FieldAccessExpr*>(root)->object;
            }
            if (root->kind == Expr::Kind::Ident) {
                auto* ident = static_cast<IdentExpr*>(root);
                if (mutable_vars_.find(ident->name) == mutable_vars_.end()) {
                    diag_.error(stmt->loc,
                        std::string("cannot assign to field of immutable binding '") +
                        std::string(ident->name) + "'; use 'var' instead of 'val'");
                    break;
                }
            }

            // Check field mutability
            Expr* penultimate = fas->target;
            if (penultimate->kind == Expr::Kind::FieldAccess) {
                auto* fa = static_cast<FieldAccessExpr*>(penultimate);
                // Find the struct type of fa->object
                auto eit = expr_struct_names_.find(fa->object);
                std::string_view sname;
                if (eit != expr_struct_names_.end()) {
                    sname = eit->second;
                } else if (fa->object->kind == Expr::Kind::Ident) {
                    sname = localStructName(static_cast<IdentExpr*>(fa->object)->name);
                }
                if (!sname.empty()) {
                    auto def_it = struct_defs_.find(sname);
                    if (def_it != struct_defs_.end()) {
                        for (const auto& fd : def_it->second.fields) {
                            if (fd.name == fa->field_name) {
                                if (!fd.is_mutable) {
                                    diag_.error(stmt->loc,
                                        std::string("cannot assign to immutable field '") +
                                        std::string(fd.name) + "' of struct '" +
                                        std::string(sname) + "'");
                                }
                                break;
                            }
                        }
                    }
                }
            }

            // Check value type matches
            Type val_type = checkExpr(fas->value, target_type);
            if (val_type != Type::Error && target_type != Type::Error && val_type != target_type) {
                diag_.error(stmt->loc,
                    std::string("field assignment type mismatch: expected ") +
                    typeName(target_type) + ", got " + typeName(val_type));
            }
            break;
        }
        case Stmt::Kind::DerefAssign: {
            auto* da = static_cast<DerefAssignStmt*>(stmt);
            Type target_type = checkExpr(da->target);

            // For *ptr = val: target is UnaryOp(Deref)
            // For (*ptr).field = val: target is FieldAccess chain rooted in Deref
            // Walk to find the root deref and check it's Ptr<var T>
            Expr* root = da->target;
            while (root->kind == Expr::Kind::FieldAccess) {
                root = static_cast<FieldAccessExpr*>(root)->object;
            }

            if (root->kind == Expr::Kind::UnaryOp) {
                auto* deref = static_cast<UnaryOpExpr*>(root);
                if (deref->op == UnaryOpKind_t::Deref) {
                    Type ptr_type = typeOfExpr(deref->operand);
                    if (ptr_type == Type::Ptr) {
                        diag_.error(stmt->loc,
                            "cannot write through 'Ptr<T>' (read-only); use 'Ptr<var T>' instead");
                        break;
                    }
                    if (ptr_type != Type::PtrVar) {
                        diag_.error(stmt->loc,
                            std::string("cannot write through non-pointer type '") +
                            typeName(ptr_type) + "'");
                        break;
                    }
                    // If target is (*ptr).field, check field mutability
                    if (da->target->kind == Expr::Kind::FieldAccess) {
                        auto* fa = static_cast<FieldAccessExpr*>(da->target);
                        // Find the struct type of the deref result
                        Type deref_type = typeOfExpr(root);
                        if (deref_type == Type::Struct) {
                            auto sn = structNameOfExpr(root);
                            if (!sn.empty()) {
                                auto def_it = struct_defs_.find(sn);
                                if (def_it != struct_defs_.end()) {
                                    for (const auto& fd : def_it->second.fields) {
                                        if (fd.name == fa->field_name && !fd.is_mutable) {
                                            diag_.error(stmt->loc,
                                                std::string("cannot assign to immutable field '") +
                                                std::string(fd.name) + "' of struct '" +
                                                std::string(sn) + "'");
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // Check value type matches target
            Type val_type = checkExpr(da->value, target_type);
            if (val_type != Type::Error && target_type != Type::Error && val_type != target_type) {
                diag_.error(stmt->loc,
                    std::string("deref assignment type mismatch: expected ") +
                    typeName(target_type) + ", got " + typeName(val_type));
            }
            break;
        }
    }
}

} // namespace kern
