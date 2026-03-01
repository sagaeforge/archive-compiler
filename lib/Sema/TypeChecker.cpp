#include "kern/sema/TypeChecker.h"
#include <string>

namespace kern {

const char* typeName(Type t) {
    switch (t) {
        case Type::I64:   return "i64";
        case Type::Bool:  return "bool";
        case Type::Unit:  return "Unit";
        case Type::Error: return "<error>";
    }
    return "<unknown>";
}

TypeChecker::TypeChecker(DiagnosticEngine& diag) : diag_(diag) {}

Type TypeChecker::resolveType(const TypeRef& ref) {
    if (ref.name == "i64")  return Type::I64;
    if (ref.name == "bool") return Type::Bool;
    if (ref.name == "Unit") return Type::Unit;
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

    switch (expr->kind) {
        case Expr::Kind::IntLit:
            return Type::I64;

        case Expr::Kind::BoolLit:
            return Type::Bool;

        case Expr::Kind::Ident: {
            auto* ident = static_cast<IdentExpr*>(expr);
            auto it = local_vars_.find(ident->name);
            if (it == local_vars_.end()) {
                diag_.error(expr->loc, std::string("undeclared identifier '") +
                            std::string(ident->name) + "'");
                return Type::Error;
            }
            return it->second;
        }

        case Expr::Kind::BinOp: {
            auto* bin = static_cast<BinOpExpr*>(expr);
            Type lhs = checkExpr(bin->lhs);
            Type rhs = checkExpr(bin->rhs);
            if (lhs == Type::Error || rhs == Type::Error) return Type::Error;

            switch (bin->op) {
                case BinOpKind::Add:
                case BinOpKind::Sub:
                case BinOpKind::Mul:
                case BinOpKind::Div:
                    if (lhs != Type::I64 || rhs != Type::I64) {
                        diag_.error(expr->loc, "arithmetic operators require i64 operands");
                        return Type::Error;
                    }
                    return Type::I64;

                case BinOpKind::Eq:
                case BinOpKind::NotEq:
                case BinOpKind::Lt:
                case BinOpKind::LtEq:
                case BinOpKind::Gt:
                case BinOpKind::GtEq:
                    if (lhs != rhs) {
                        diag_.error(expr->loc, "comparison operators require same-type operands");
                        return Type::Error;
                    }
                    return Type::Bool;

                case BinOpKind::And:
                case BinOpKind::Or:
                    if (lhs != Type::Bool || rhs != Type::Bool) {
                        diag_.error(expr->loc, "'and'/'or' require bool operands");
                        return Type::Error;
                    }
                    return Type::Bool;
            }
            return Type::Error;
        }

        case Expr::Kind::UnaryOp: {
            auto* unary = static_cast<UnaryOpExpr*>(expr);
            Type operand = checkExpr(unary->operand);
            if (operand == Type::Error) return Type::Error;

            switch (unary->op) {
                case UnaryOpKind_t::Neg:
                    if (operand != Type::I64) {
                        diag_.error(expr->loc, "unary '-' requires i64 operand");
                        return Type::Error;
                    }
                    return Type::I64;
                case UnaryOpKind_t::Not:
                    if (operand != Type::Bool) {
                        diag_.error(expr->loc, "'not' requires bool operand");
                        return Type::Error;
                    }
                    return Type::Bool;
                default:
                    return Type::Error;
            }
        }

        case Expr::Kind::Call: {
            auto* call = static_cast<CallExpr*>(expr);
            auto it = fn_table_.find(call->callee);
            if (it == fn_table_.end()) {
                diag_.error(expr->loc, std::string("undeclared function '") +
                            std::string(call->callee) + "'");
                return Type::Error;
            }
            const FnSig& sig = it->second;
            if (call->arg_count != sig.param_types.size()) {
                diag_.error(expr->loc, std::string("function '") + std::string(call->callee) +
                            "' expects " + std::to_string(sig.param_types.size()) +
                            " arguments, got " + std::to_string(call->arg_count));
                return Type::Error;
            }
            for (uint32_t i = 0; i < call->arg_count; ++i) {
                Type arg = checkExpr(call->args[i]);
                if (arg != Type::Error && arg != sig.param_types[i]) {
                    diag_.error(call->args[i]->loc,
                                std::string("argument type mismatch: expected ") +
                                typeName(sig.param_types[i]) + ", got " + typeName(arg));
                }
            }
            return sig.return_type;
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
                    return Type::Error;
                }
                return then_t;
            }
            return then_t;
        }

        case Expr::Kind::Block:
            return checkBlock(static_cast<BlockExpr*>(expr));

        case Expr::Kind::Return: {
            auto* ret = static_cast<ReturnExpr*>(expr);
            if (ret->value) {
                Type val = checkExpr(ret->value);
                if (val != Type::Error && val != current_return_type_) {
                    diag_.error(expr->loc, std::string("return type mismatch: expected ") +
                                typeName(current_return_type_) + ", got " + typeName(val));
                }
                return val;
            }
            return Type::Unit;
        }
    }
    return Type::Error;
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
            diag_.warning(stmt->loc, "var makes function impure; consider using val + recursion");
            break;
        }
        case Stmt::Kind::ExprStmt:
            checkExpr(static_cast<ExprStmt*>(stmt)->expr);
            break;
    }
}

} // namespace kern
