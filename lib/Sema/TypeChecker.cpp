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

bool isStruct(Type t) {
    return t == Type::Struct;
}

int sizeBytes(Type t) {
    switch (t) {
        case Type::I8:  case Type::U8:  case Type::Bool: return 1;
        case Type::I16: case Type::U16: return 2;
        case Type::I32: case Type::U32: case Type::F32: return 4;
        case Type::I64: case Type::U64: case Type::F64: return 8;
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

TypeChecker::TypeChecker(DiagnosticEngine& diag) : diag_(diag) {}

Type TypeChecker::resolveType(const TypeRef& ref) {
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
    if (struct_defs_.count(ref.name)) return Type::Struct;
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

    // First pass: register all function signatures
    for (uint32_t i = 0; i < mod->fn_count; ++i) {
        FnDecl* fn = mod->functions[i];
        FnSig sig;
        sig.name = fn->name;
        sig.return_type = resolveType(fn->return_type);
        if (sig.return_type == Type::Struct) {
            sig.return_struct_name = fn->return_type.name;
        }
        for (uint32_t j = 0; j < fn->param_count; ++j) {
            Type pt = resolveType(fn->params[j].type);
            sig.param_types.push_back(pt);
            sig.param_struct_names.push_back(
                pt == Type::Struct ? fn->params[j].type.name : std::string_view{});
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
                    if (!sn.empty()) {
                        expr_struct_names_[expr] = sn;
                    }
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
                default:
                    result = Type::Error;
                    break;
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

        case Expr::Kind::Match: {
            auto* matchE = static_cast<MatchExpr*>(expr);
            Type scrut_type = checkExpr(matchE->scrutinee);
            if (scrut_type == Type::Error) {
                result = Type::Error;
                break;
            }

            // Save/restore local vars for arm scoping
            auto saved_locals = local_vars_;

            Type arm_type = Type::Error;
            bool has_catch_all = false;
            bool has_true = false, has_false = false;

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
                        local_vars_[vp->name] = scrut_type;
                        if (!arm.guard) has_catch_all = true;
                        break;
                    }
                    case Pattern::Kind::Wildcard:
                        if (!arm.guard) has_catch_all = true;
                        break;
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
    }
}

} // namespace kern
