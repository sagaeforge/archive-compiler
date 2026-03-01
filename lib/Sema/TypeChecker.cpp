#include "kern/sema/TypeChecker.h"
#include <string>

namespace kern {

const char* typeName(Type t) {
    switch (t) {
        case Type::I8:    return "i8";
        case Type::I16:   return "i16";
        case Type::I32:   return "i32";
        case Type::I64:   return "i64";
        case Type::U8:    return "u8";
        case Type::U16:   return "u16";
        case Type::U32:   return "u32";
        case Type::U64:   return "u64";
        case Type::Bool:  return "bool";
        case Type::Unit:  return "Unit";
        case Type::Error: return "<error>";
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

int bitWidth(Type t) {
    switch (t) {
        case Type::I8:  case Type::U8:  return 8;
        case Type::I16: case Type::U16: return 16;
        case Type::I32: case Type::U32: return 32;
        case Type::I64: case Type::U64: return 64;
        case Type::Bool: return 1;
        default: return 0;
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
    if (ref.name == "bool") return Type::Bool;
    if (ref.name == "Unit") return Type::Unit;
    diag_.error({}, std::string("unknown type '") + std::string(ref.name) + "'");
    return Type::Error;
}

Type TypeChecker::typeOfExpr(const Expr* expr) const {
    auto it = expr_types_.find(expr);
    if (it != expr_types_.end()) return it->second;
    return Type::Error;
}

bool TypeChecker::check(Module* mod) {
    // First pass: register all function signatures
    for (uint32_t i = 0; i < mod->fn_count; ++i) {
        FnDecl* fn = mod->functions[i];
        FnSig sig;
        sig.name = fn->name;
        sig.return_type = resolveType(fn->return_type);
        for (uint32_t j = 0; j < fn->param_count; ++j) {
            sig.param_types.push_back(resolveType(fn->params[j].type));
        }
        fn_table_[fn->name] = sig;
    }

    // Second pass: type-check function bodies
    for (uint32_t i = 0; i < mod->fn_count; ++i) {
        checkFn(mod->functions[i]);
    }

    return !diag_.hasErrors();
}

Type TypeChecker::checkFn(FnDecl* fn) {
    local_vars_.clear();
    current_return_type_ = resolveType(fn->return_type);

    // Register parameters
    for (uint32_t i = 0; i < fn->param_count; ++i) {
        Type pt = resolveType(fn->params[i].type);
        local_vars_[fn->params[i].name] = pt;
    }

    Type body_type = checkExpr(fn->body);

    if (body_type != Type::Error && body_type != current_return_type_) {
        diag_.error(fn->loc, std::string("function '") + std::string(fn->name) +
                    "' declared to return " + typeName(current_return_type_) +
                    " but body has type " + typeName(body_type));
    }

    return body_type;
}

Type TypeChecker::checkExpr(Expr* expr) {
    if (!expr) return Type::Unit;

    Type result = Type::Error;

    switch (expr->kind) {
        case Expr::Kind::IntLit:
            result = Type::I64;
            break;

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
            }
            break;
        }

        case Expr::Kind::BinOp: {
            auto* bin = static_cast<BinOpExpr*>(expr);
            Type lhs = checkExpr(bin->lhs);
            Type rhs = checkExpr(bin->rhs);
            if (lhs == Type::Error || rhs == Type::Error) {
                result = Type::Error;
                break;
            }

            switch (bin->op) {
                case BinOpKind::Add:
                case BinOpKind::Sub:
                case BinOpKind::Mul:
                case BinOpKind::Div:
                    if (!isInteger(lhs) || lhs != rhs) {
                        diag_.error(expr->loc,
                            std::string("arithmetic operators require same integer type operands, got ") +
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
            Type operand = checkExpr(unary->operand);
            if (operand == Type::Error) {
                result = Type::Error;
                break;
            }

            switch (unary->op) {
                case UnaryOpKind_t::Neg:
                    if (!isSigned(operand)) {
                        diag_.error(expr->loc,
                            std::string("unary '-' requires signed integer operand, got ") +
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
                Type arg = checkExpr(call->args[i]);
                if (arg != Type::Error && arg != sig.param_types[i]) {
                    diag_.error(call->args[i]->loc,
                                std::string("argument type mismatch: expected ") +
                                typeName(sig.param_types[i]) + ", got " + typeName(arg));
                }
            }
            result = sig.return_type;
            break;
        }

        case Expr::Kind::If: {
            auto* ifE = static_cast<IfExpr*>(expr);
            Type cond = checkExpr(ifE->condition);
            if (cond != Type::Error && cond != Type::Bool) {
                diag_.error(ifE->condition->loc, "if condition must be bool");
            }
            Type then_t = checkExpr(ifE->then_branch);
            if (ifE->else_branch) {
                Type else_t = checkExpr(ifE->else_branch);
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
            result = checkBlock(static_cast<BlockExpr*>(expr));
            break;

        case Expr::Kind::Return: {
            auto* ret = static_cast<ReturnExpr*>(expr);
            if (ret->value) {
                Type val = checkExpr(ret->value);
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
    }

    expr_types_[expr] = result;
    return result;
}

Type TypeChecker::checkBlock(BlockExpr* block) {
    for (uint32_t i = 0; i < block->stmt_count; ++i) {
        checkStmt(block->stmts[i]);
    }
    if (block->result) {
        return checkExpr(block->result);
    }
    return Type::Unit;
}

void TypeChecker::checkStmt(Stmt* stmt) {
    switch (stmt->kind) {
        case Stmt::Kind::ValDecl: {
            auto* decl = static_cast<ValDeclStmt*>(stmt);
            Type expected = resolveType(decl->type);
            Type actual = checkExpr(decl->init);
            if (actual != Type::Error && actual != expected) {
                diag_.error(stmt->loc, std::string("val type mismatch: expected ") +
                            typeName(expected) + ", got " + typeName(actual));
            }
            local_vars_[decl->name] = expected;
            break;
        }
        case Stmt::Kind::VarDecl: {
            auto* decl = static_cast<VarDeclStmt*>(stmt);
            Type expected = resolveType(decl->type);
            Type actual = checkExpr(decl->init);
            if (actual != Type::Error && actual != expected) {
                diag_.error(stmt->loc, std::string("var type mismatch: expected ") +
                            typeName(expected) + ", got " + typeName(actual));
            }
            local_vars_[decl->name] = expected;
            // var impurity warning is now emitted by PurityChecker
            break;
        }
        case Stmt::Kind::ExprStmt:
            checkExpr(static_cast<ExprStmt*>(stmt)->expr);
            break;
    }
}

} // namespace kern
