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
        case Stmt::Kind::ExprStmt:
            collectCallees(static_cast<ExprStmt*>(stmt)->expr, callees);
            break;
    }
}

bool PurityChecker::bodyUsesVar(FnDecl* fn) const {
    // Walk the body looking for VarDecl statements
    if (!fn->body) return false;

    // The body should be a BlockExpr
    if (fn->body->kind != Expr::Kind::Block) return false;

    auto* block = static_cast<BlockExpr*>(fn->body);
    for (uint32_t i = 0; i < block->stmt_count; ++i) {
        if (block->stmts[i]->kind == Stmt::Kind::VarDecl) {
            return true;
        }
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

        // If no impurity found, it's pure
        if (!pr.uses_var && pr.purity == Purity::Pure) {
            pr.purity = Purity::Pure;
        }

        // Check recursion
        pr.is_recursive = isRecursive(fn_name);

        results[fn_name] = pr;
    }

    return results;
}

} // namespace kern
