#include "kern/sema/PurityChecker.h"
#include <algorithm>
#include <stack>

namespace kern {

PurityChecker::PurityChecker(DiagnosticEngine& diag) : diag_(diag) {}

void PurityChecker::collectCallees(Expr* expr, std::unordered_set<std::string_view>& callees) const {
    if (!expr) return;

    switch (expr->kind) {
        case Expr::Kind::Call: {
            auto* call = static_cast<CallExpr*>(expr);
            callees.insert(call->callee);
            for (uint32_t i = 0; i < call->arg_count; ++i) {
                collectCallees(call->args[i], callees);
            }
            break;
        }
        case Expr::Kind::BinOp: {
            auto* bin = static_cast<BinOpExpr*>(expr);
            collectCallees(bin->lhs, callees);
            collectCallees(bin->rhs, callees);
            break;
        }
        case Expr::Kind::UnaryOp: {
            auto* unary = static_cast<UnaryOpExpr*>(expr);
            collectCallees(unary->operand, callees);
            break;
        }
        case Expr::Kind::If: {
            auto* ifE = static_cast<IfExpr*>(expr);
            collectCallees(ifE->condition, callees);
            collectCallees(ifE->then_branch, callees);
            if (ifE->else_branch) collectCallees(ifE->else_branch, callees);
            break;
        }
        case Expr::Kind::Block: {
            auto* block = static_cast<BlockExpr*>(expr);
            for (uint32_t i = 0; i < block->stmt_count; ++i) {
                collectCalleesStmt(block->stmts[i], callees);
            }
            if (block->result) collectCallees(block->result, callees);
            break;
        }
        case Expr::Kind::Return: {
            auto* ret = static_cast<ReturnExpr*>(expr);
            if (ret->value) collectCallees(ret->value, callees);
            break;
        }
        case Expr::Kind::Match: {
            auto* m = static_cast<MatchExpr*>(expr);
            collectCallees(m->scrutinee, callees);
            for (uint32_t i = 0; i < m->arm_count; ++i) {
                if (m->arms[i].guard) collectCallees(m->arms[i].guard, callees);
                collectCallees(m->arms[i].body, callees);
            }
            break;
        }
        case Expr::Kind::StructLit: {
            auto* sl = static_cast<StructLitExpr*>(expr);
            for (uint32_t i = 0; i < sl->field_count; ++i) {
                collectCallees(sl->fields[i].value, callees);
            }
            break;
        }
        case Expr::Kind::FieldAccess: {
            auto* fa = static_cast<FieldAccessExpr*>(expr);
            collectCallees(fa->object, callees);
            break;
        }
        default:
            break;
    }
}

void PurityChecker::collectCalleesStmt(Stmt* stmt, std::unordered_set<std::string_view>& callees) const {
    switch (stmt->kind) {
        case Stmt::Kind::ValDecl: {
            auto* decl = static_cast<ValDeclStmt*>(stmt);
            collectCallees(decl->init, callees);
            break;
        }
        case Stmt::Kind::VarDecl: {
            auto* decl = static_cast<VarDeclStmt*>(stmt);
            collectCallees(decl->init, callees);
            break;
        }
        case Stmt::Kind::Assign: {
            auto* assign = static_cast<AssignStmt*>(stmt);
            collectCallees(assign->value, callees);
            break;
        }
        case Stmt::Kind::ExprStmt:
            collectCallees(static_cast<ExprStmt*>(stmt)->expr, callees);
            break;
        case Stmt::Kind::FieldAssign: {
            auto* fas = static_cast<FieldAssignStmt*>(stmt);
            collectCallees(fas->target, callees);
            collectCallees(fas->value, callees);
            break;
        }
    }
}

bool PurityChecker::bodyUsesVar(FnDecl* fn) const {
    if (!fn->body) return false;
    return exprUsesVar(fn->body);
}

bool PurityChecker::exprUsesVar(Expr* expr) const {
    if (!expr) return false;

    switch (expr->kind) {
        case Expr::Kind::Block: {
            auto* block = static_cast<BlockExpr*>(expr);
            for (uint32_t i = 0; i < block->stmt_count; ++i) {
                if (stmtUsesVar(block->stmts[i])) return true;
            }
            if (block->result && exprUsesVar(block->result)) return true;
            return false;
        }
        case Expr::Kind::If: {
            auto* ifE = static_cast<IfExpr*>(expr);
            if (exprUsesVar(ifE->condition)) return true;
            if (exprUsesVar(ifE->then_branch)) return true;
            if (ifE->else_branch && exprUsesVar(ifE->else_branch)) return true;
            return false;
        }
        case Expr::Kind::BinOp: {
            auto* bin = static_cast<BinOpExpr*>(expr);
            return exprUsesVar(bin->lhs) || exprUsesVar(bin->rhs);
        }
        case Expr::Kind::UnaryOp: {
            auto* unary = static_cast<UnaryOpExpr*>(expr);
            return exprUsesVar(unary->operand);
        }
        case Expr::Kind::Call: {
            auto* call = static_cast<CallExpr*>(expr);
            for (uint32_t i = 0; i < call->arg_count; ++i) {
                if (exprUsesVar(call->args[i])) return true;
            }
            return false;
        }
        case Expr::Kind::Return: {
            auto* ret = static_cast<ReturnExpr*>(expr);
            return ret->value && exprUsesVar(ret->value);
        }
        case Expr::Kind::Match: {
            auto* m = static_cast<MatchExpr*>(expr);
            if (exprUsesVar(m->scrutinee)) return true;
            for (uint32_t i = 0; i < m->arm_count; ++i) {
                if (m->arms[i].guard && exprUsesVar(m->arms[i].guard)) return true;
                if (exprUsesVar(m->arms[i].body)) return true;
            }
            return false;
        }
        case Expr::Kind::StructLit: {
            auto* sl = static_cast<StructLitExpr*>(expr);
            for (uint32_t i = 0; i < sl->field_count; ++i) {
                if (exprUsesVar(sl->fields[i].value)) return true;
            }
            return false;
        }
        case Expr::Kind::FieldAccess: {
            auto* fa = static_cast<FieldAccessExpr*>(expr);
            return exprUsesVar(fa->object);
        }
        default:
            return false;
    }
}

bool PurityChecker::stmtUsesVar(Stmt* stmt) const {
    if (stmt->kind == Stmt::Kind::VarDecl) return true;
    if (stmt->kind == Stmt::Kind::Assign) return true; // mutation
    if (stmt->kind == Stmt::Kind::FieldAssign) return true; // mutation
    if (stmt->kind == Stmt::Kind::ValDecl) {
        auto* decl = static_cast<ValDeclStmt*>(stmt);
        return exprUsesVar(decl->init);
    }
    if (stmt->kind == Stmt::Kind::ExprStmt) {
        return exprUsesVar(static_cast<ExprStmt*>(stmt)->expr);
    }
    return false;
}

void PurityChecker::buildCallGraph(Module* mod) {
    fn_map_.clear();
    call_graph_.clear();

    for (uint32_t i = 0; i < mod->fn_count; ++i) {
        FnDecl* fn = mod->functions[i];
        fn_map_[fn->name] = fn;

        std::unordered_set<std::string_view> callees;
        collectCallees(fn->body, callees);
        call_graph_[fn->name] = std::move(callees);
    }
}

bool PurityChecker::isTailRecursive(std::string_view fn_name, FnDecl* fn) const {
    if (!fn->body) return false;
    // A function is tail-recursive if:
    // 1. It has at least one recursive call in tail position
    // 2. It has NO recursive calls in non-tail positions
    return exprHasTailCall(fn->body, fn_name) &&
           !exprHasNonTailCall(fn->body, fn_name);
}

bool PurityChecker::exprHasTailCall(Expr* expr, std::string_view fn_name) const {
    if (!expr) return false;

    switch (expr->kind) {
        case Expr::Kind::Call: {
            auto* call = static_cast<CallExpr*>(expr);
            return call->callee == fn_name;
        }
        case Expr::Kind::Block: {
            auto* block = static_cast<BlockExpr*>(expr);
            if (block->result) {
                return exprHasTailCall(block->result, fn_name);
            }
            if (block->stmt_count > 0) {
                auto* last = block->stmts[block->stmt_count - 1];
                if (last->kind == Stmt::Kind::ExprStmt) {
                    return exprHasTailCall(static_cast<ExprStmt*>(last)->expr, fn_name);
                }
            }
            return false;
        }
        case Expr::Kind::If: {
            auto* ifE = static_cast<IfExpr*>(expr);
            bool then_tail = exprHasTailCall(ifE->then_branch, fn_name);
            bool else_tail = ifE->else_branch && exprHasTailCall(ifE->else_branch, fn_name);
            return then_tail || else_tail;
        }
        case Expr::Kind::Return: {
            auto* ret = static_cast<ReturnExpr*>(expr);
            return ret->value && exprHasTailCall(ret->value, fn_name);
        }
        case Expr::Kind::Match: {
            auto* m = static_cast<MatchExpr*>(expr);
            for (uint32_t i = 0; i < m->arm_count; ++i) {
                if (exprHasTailCall(m->arms[i].body, fn_name)) return true;
            }
            return false;
        }
        default:
            return false;
    }
}

// Check if expr in NON-tail position contains any recursive calls
bool PurityChecker::exprHasNonTailCall(Expr* expr, std::string_view fn_name) const {
    if (!expr) return false;

    switch (expr->kind) {
        case Expr::Kind::Call: {
            // A call in tail position is fine — but check its arguments (non-tail)
            auto* call = static_cast<CallExpr*>(expr);
            for (uint32_t i = 0; i < call->arg_count; ++i) {
                if (exprHasNonTailCallInner(call->args[i], fn_name)) return true;
            }
            return false;
        }
        case Expr::Kind::Block: {
            auto* block = static_cast<BlockExpr*>(expr);
            // Statements are non-tail positions
            for (uint32_t i = 0; i < block->stmt_count; ++i) {
                auto* st = block->stmts[i];
                if (st->kind == Stmt::Kind::ExprStmt) {
                    if (exprHasNonTailCallInner(static_cast<ExprStmt*>(st)->expr, fn_name))
                        return true;
                } else if (st->kind == Stmt::Kind::ValDecl) {
                    if (exprHasNonTailCallInner(static_cast<ValDeclStmt*>(st)->init, fn_name))
                        return true;
                } else if (st->kind == Stmt::Kind::VarDecl) {
                    if (exprHasNonTailCallInner(static_cast<VarDeclStmt*>(st)->init, fn_name))
                        return true;
                } else if (st->kind == Stmt::Kind::Assign) {
                    if (exprHasNonTailCallInner(static_cast<AssignStmt*>(st)->value, fn_name))
                        return true;
                } else if (st->kind == Stmt::Kind::FieldAssign) {
                    auto* fas = static_cast<FieldAssignStmt*>(st);
                    if (exprHasNonTailCallInner(fas->target, fn_name)) return true;
                    if (exprHasNonTailCallInner(fas->value, fn_name)) return true;
                }
            }
            // Result is a tail position — recurse with tail-aware check
            if (block->result) {
                return exprHasNonTailCall(block->result, fn_name);
            }
            return false;
        }
        case Expr::Kind::If: {
            auto* ifE = static_cast<IfExpr*>(expr);
            // Condition is non-tail
            if (exprHasNonTailCallInner(ifE->condition, fn_name)) return true;
            // Branches are tail positions
            if (exprHasNonTailCall(ifE->then_branch, fn_name)) return true;
            if (ifE->else_branch && exprHasNonTailCall(ifE->else_branch, fn_name)) return true;
            return false;
        }
        case Expr::Kind::Return: {
            auto* ret = static_cast<ReturnExpr*>(expr);
            if (ret->value) return exprHasNonTailCall(ret->value, fn_name);
            return false;
        }
        case Expr::Kind::BinOp: {
            // Both sides of binop are non-tail
            auto* bin = static_cast<BinOpExpr*>(expr);
            return exprHasNonTailCallInner(bin->lhs, fn_name) ||
                   exprHasNonTailCallInner(bin->rhs, fn_name);
        }
        case Expr::Kind::UnaryOp: {
            auto* unary = static_cast<UnaryOpExpr*>(expr);
            return exprHasNonTailCallInner(unary->operand, fn_name);
        }
        case Expr::Kind::Match: {
            auto* m = static_cast<MatchExpr*>(expr);
            // Scrutinee is non-tail
            if (exprHasNonTailCallInner(m->scrutinee, fn_name)) return true;
            // Each arm body is a tail position; guards are non-tail
            for (uint32_t i = 0; i < m->arm_count; ++i) {
                if (m->arms[i].guard && exprHasNonTailCallInner(m->arms[i].guard, fn_name))
                    return true;
                if (exprHasNonTailCall(m->arms[i].body, fn_name)) return true;
            }
            return false;
        }
        case Expr::Kind::StructLit: {
            auto* sl = static_cast<StructLitExpr*>(expr);
            for (uint32_t i = 0; i < sl->field_count; ++i) {
                if (exprHasNonTailCallInner(sl->fields[i].value, fn_name)) return true;
            }
            return false;
        }
        case Expr::Kind::FieldAccess: {
            auto* fa = static_cast<FieldAccessExpr*>(expr);
            return exprHasNonTailCallInner(fa->object, fn_name);
        }
        default:
            return false;
    }
}

// Check if ANY recursive call exists (regardless of position) — for non-tail contexts
bool PurityChecker::exprHasNonTailCallInner(Expr* expr, std::string_view fn_name) const {
    if (!expr) return false;

    switch (expr->kind) {
        case Expr::Kind::Call: {
            auto* call = static_cast<CallExpr*>(expr);
            if (call->callee == fn_name) return true;
            for (uint32_t i = 0; i < call->arg_count; ++i) {
                if (exprHasNonTailCallInner(call->args[i], fn_name)) return true;
            }
            return false;
        }
        case Expr::Kind::BinOp: {
            auto* bin = static_cast<BinOpExpr*>(expr);
            return exprHasNonTailCallInner(bin->lhs, fn_name) ||
                   exprHasNonTailCallInner(bin->rhs, fn_name);
        }
        case Expr::Kind::UnaryOp: {
            auto* unary = static_cast<UnaryOpExpr*>(expr);
            return exprHasNonTailCallInner(unary->operand, fn_name);
        }
        case Expr::Kind::If: {
            auto* ifE = static_cast<IfExpr*>(expr);
            if (exprHasNonTailCallInner(ifE->condition, fn_name)) return true;
            if (exprHasNonTailCallInner(ifE->then_branch, fn_name)) return true;
            if (ifE->else_branch && exprHasNonTailCallInner(ifE->else_branch, fn_name)) return true;
            return false;
        }
        case Expr::Kind::Block: {
            auto* block = static_cast<BlockExpr*>(expr);
            for (uint32_t i = 0; i < block->stmt_count; ++i) {
                auto* st = block->stmts[i];
                if (st->kind == Stmt::Kind::ExprStmt) {
                    if (exprHasNonTailCallInner(static_cast<ExprStmt*>(st)->expr, fn_name))
                        return true;
                } else if (st->kind == Stmt::Kind::ValDecl) {
                    if (exprHasNonTailCallInner(static_cast<ValDeclStmt*>(st)->init, fn_name))
                        return true;
                } else if (st->kind == Stmt::Kind::VarDecl) {
                    if (exprHasNonTailCallInner(static_cast<VarDeclStmt*>(st)->init, fn_name))
                        return true;
                } else if (st->kind == Stmt::Kind::Assign) {
                    if (exprHasNonTailCallInner(static_cast<AssignStmt*>(st)->value, fn_name))
                        return true;
                } else if (st->kind == Stmt::Kind::FieldAssign) {
                    auto* fas = static_cast<FieldAssignStmt*>(st);
                    if (exprHasNonTailCallInner(fas->target, fn_name)) return true;
                    if (exprHasNonTailCallInner(fas->value, fn_name)) return true;
                }
            }
            if (block->result) return exprHasNonTailCallInner(block->result, fn_name);
            return false;
        }
        case Expr::Kind::Return: {
            auto* ret = static_cast<ReturnExpr*>(expr);
            return ret->value && exprHasNonTailCallInner(ret->value, fn_name);
        }
        case Expr::Kind::Match: {
            auto* m = static_cast<MatchExpr*>(expr);
            if (exprHasNonTailCallInner(m->scrutinee, fn_name)) return true;
            for (uint32_t i = 0; i < m->arm_count; ++i) {
                if (m->arms[i].guard && exprHasNonTailCallInner(m->arms[i].guard, fn_name))
                    return true;
                if (exprHasNonTailCallInner(m->arms[i].body, fn_name)) return true;
            }
            return false;
        }
        case Expr::Kind::StructLit: {
            auto* sl = static_cast<StructLitExpr*>(expr);
            for (uint32_t i = 0; i < sl->field_count; ++i) {
                if (exprHasNonTailCallInner(sl->fields[i].value, fn_name)) return true;
            }
            return false;
        }
        case Expr::Kind::FieldAccess: {
            auto* fa = static_cast<FieldAccessExpr*>(expr);
            return exprHasNonTailCallInner(fa->object, fn_name);
        }
        default:
            return false;
    }
}

bool PurityChecker::isRecursive(std::string_view fn_name) const {
    // Direct self-recursion
    auto it = call_graph_.find(fn_name);
    if (it == call_graph_.end()) return false;
    if (it->second.count(fn_name) > 0) return true;

    // Mutual recursion: check if fn_name is reachable from any of its callees
    std::unordered_set<std::string_view> visited;
    std::stack<std::string_view> worklist;
    for (auto& callee : it->second) {
        if (callee != fn_name) {
            worklist.push(callee);
        }
    }
    while (!worklist.empty()) {
        auto current = worklist.top();
        worklist.pop();
        if (current == fn_name) return true;
        if (visited.count(current) > 0) continue;
        visited.insert(current);

        auto cit = call_graph_.find(current);
        if (cit != call_graph_.end()) {
            for (auto& callee : cit->second) {
                worklist.push(callee);
            }
        }
    }
    return false;
}

std::vector<std::string_view> PurityChecker::reverseTopoSort() const {
    // Kahn's algorithm for topological sort, then reverse
    std::unordered_map<std::string_view, int> in_degree;
    for (auto& [name, _] : fn_map_) {
        in_degree[name] = 0;
    }

    for (auto& [caller, callees] : call_graph_) {
        for (auto& callee : callees) {
            if (fn_map_.count(callee) > 0 && callee != caller) {
                in_degree[callee]++;
            }
        }
    }

    std::vector<std::string_view> queue;
    for (auto& [name, deg] : in_degree) {
        if (deg == 0) queue.push_back(name);
    }

    std::vector<std::string_view> result;
    size_t idx = 0;
    while (idx < queue.size()) {
        auto current = queue[idx++];
        result.push_back(current);

        auto cit = call_graph_.find(current);
        if (cit != call_graph_.end()) {
            for (auto& callee : cit->second) {
                if (fn_map_.count(callee) > 0 && callee != current) {
                    in_degree[callee]--;
                    if (in_degree[callee] == 0) {
                        queue.push_back(callee);
                    }
                }
            }
        }
    }

    // Add any remaining (cycle members) not yet in result
    for (auto& [name, _] : fn_map_) {
        if (std::find(result.begin(), result.end(), name) == result.end()) {
            result.push_back(name);
        }
    }

    // Reverse: leaves first (functions with no callees)
    std::reverse(result.begin(), result.end());
    return result;
}

std::unordered_map<std::string_view, PurityResult> PurityChecker::analyze(Module* mod) {
    buildCallGraph(mod);

    std::unordered_map<std::string_view, PurityResult> results;

    // Process in reverse topological order (leaves first)
    auto order = reverseTopoSort();

    for (auto fn_name : order) {
        PurityResult pr;
        FnDecl* fn = fn_map_[fn_name];

        // Intrinsic functions are classified as ImpureIo
        if (fn->is_intrinsic) {
            pr.purity = Purity::ImpureIo;
            results[fn_name] = pr;
            continue;
        }

        // Check for var usage
        if (bodyUsesVar(fn)) {
            pr.uses_var = true;
            pr.purity = Purity::ImpureMut;

            // Emit warning for var usage
            diag_.warning(fn->loc,
                std::string("function '") + std::string(fn_name) +
                "' uses 'var' — classified as impure(mut); consider val + recursion");
        }

        // Check callees for impurity propagation (io and mem propagate, mut does not)
        auto& callees = call_graph_[fn_name];
        for (auto& callee : callees) {
            auto cit = results.find(callee);
            if (cit != results.end()) {
                if (cit->second.purity == Purity::ImpureIo) {
                    pr.purity = Purity::ImpureIo;
                } else if (cit->second.purity == Purity::ImpureMem &&
                           pr.purity != Purity::ImpureIo) {
                    pr.purity = Purity::ImpureMem;
                }
                // ImpureMut does NOT propagate
            }
        }

        // Check recursion and tail recursion
        pr.is_recursive = isRecursive(fn_name);
        if (pr.is_recursive) {
            pr.is_tailrec = isTailRecursive(fn_name, fn);
        }

        results[fn_name] = pr;
    }

    return results;
}

} // namespace kern
