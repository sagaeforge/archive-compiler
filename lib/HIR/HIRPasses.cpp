#include "kern/hir/HIRPasses.h"
#include "kern/support/CompilationContext.h"
#include "kern/ir/Metadata.h"
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace kern {

// ============================================================================
// Helper: collect callees from HIR expressions/statements
// ============================================================================

static void collectCallees(const HIRExpr* expr,
                           std::unordered_set<std::string_view>& out);
static void collectCalleesStmt(const HIRStmt* stmt,
                               std::unordered_set<std::string_view>& out);

static void collectCallees(const HIRExpr* expr,
                           std::unordered_set<std::string_view>& out) {
    if (!expr) return;
    switch (expr->kind) {
        case HIRExpr::Kind::IntLit:
        case HIRExpr::Kind::FloatLit:
        case HIRExpr::Kind::BoolLit:
        case HIRExpr::Kind::StringLit:
        case HIRExpr::Kind::Ident:
        case HIRExpr::Kind::EnumAccess:
            break;
        case HIRExpr::Kind::BinOp: {
            auto* e = static_cast<const HIRBinOpExpr*>(expr);
            collectCallees(e->lhs, out);
            collectCallees(e->rhs, out);
            break;
        }
        case HIRExpr::Kind::UnaryOp: {
            auto* e = static_cast<const HIRUnaryOpExpr*>(expr);
            collectCallees(e->operand, out);
            break;
        }
        case HIRExpr::Kind::Call: {
            auto* e = static_cast<const HIRCallExpr*>(expr);
            out.insert(e->callee);
            for (uint32_t i = 0; i < e->arg_count; ++i)
                collectCallees(e->args[i], out);
            break;
        }
        case HIRExpr::Kind::If: {
            auto* e = static_cast<const HIRIfExpr*>(expr);
            collectCallees(e->condition, out);
            collectCallees(e->then_branch, out);
            collectCallees(e->else_branch, out);
            break;
        }
        case HIRExpr::Kind::Match: {
            auto* e = static_cast<const HIRMatchExpr*>(expr);
            collectCallees(e->scrutinee, out);
            for (uint32_t i = 0; i < e->arm_count; ++i) {
                if (e->arms[i].guard) collectCallees(e->arms[i].guard, out);
                collectCallees(e->arms[i].body, out);
            }
            break;
        }
        case HIRExpr::Kind::Block: {
            auto* e = static_cast<const HIRBlockExpr*>(expr);
            for (uint32_t i = 0; i < e->stmt_count; ++i)
                collectCalleesStmt(e->stmts[i], out);
            collectCallees(e->result, out);
            break;
        }
        case HIRExpr::Kind::Return: {
            auto* e = static_cast<const HIRReturnExpr*>(expr);
            collectCallees(e->value, out);
            break;
        }
        case HIRExpr::Kind::StructLit: {
            auto* e = static_cast<const HIRStructLitExpr*>(expr);
            for (uint32_t i = 0; i < e->field_count; ++i)
                collectCallees(e->fields[i].value, out);
            break;
        }
        case HIRExpr::Kind::FieldAccess: {
            auto* e = static_cast<const HIRFieldAccessExpr*>(expr);
            collectCallees(e->object, out);
            break;
        }
        case HIRExpr::Kind::UnionVariant: {
            auto* e = static_cast<const HIRUnionVariantExpr*>(expr);
            collectCallees(e->payload, out);
            break;
        }
        case HIRExpr::Kind::AddrOf: {
            auto* e = static_cast<const HIRAddrOfExpr*>(expr);
            collectCallees(e->operand, out);
            break;
        }
        case HIRExpr::Kind::Deref: {
            auto* e = static_cast<const HIRDerefExpr*>(expr);
            collectCallees(e->operand, out);
            break;
        }
    }
}

static void collectCalleesStmt(const HIRStmt* stmt,
                               std::unordered_set<std::string_view>& out) {
    if (!stmt) return;
    switch (stmt->kind) {
        case HIRStmt::Kind::ValDecl:
            collectCallees(static_cast<const HIRValDeclStmt*>(stmt)->init, out);
            break;
        case HIRStmt::Kind::VarDecl:
            collectCallees(static_cast<const HIRVarDeclStmt*>(stmt)->init, out);
            break;
        case HIRStmt::Kind::ExprStmt:
            collectCallees(static_cast<const HIRExprStmt*>(stmt)->expr, out);
            break;
        case HIRStmt::Kind::Assign:
            collectCallees(static_cast<const HIRAssignStmt*>(stmt)->value, out);
            break;
        case HIRStmt::Kind::FieldAssign: {
            auto* s = static_cast<const HIRFieldAssignStmt*>(stmt);
            collectCallees(s->target, out);
            collectCallees(s->value, out);
            break;
        }
        case HIRStmt::Kind::DerefAssign: {
            auto* s = static_cast<const HIRDerefAssignStmt*>(stmt);
            collectCallees(s->target, out);
            collectCallees(s->value, out);
            break;
        }
    }
}

// ============================================================================
// Helper: check if HIR function body uses var bindings/mutation
// ============================================================================

static bool exprUsesVar(const HIRExpr* expr);
static bool stmtUsesVar(const HIRStmt* stmt);

static bool exprUsesVar(const HIRExpr* expr) {
    if (!expr) return false;
    switch (expr->kind) {
        case HIRExpr::Kind::IntLit:
        case HIRExpr::Kind::FloatLit:
        case HIRExpr::Kind::BoolLit:
        case HIRExpr::Kind::StringLit:
        case HIRExpr::Kind::Ident:
        case HIRExpr::Kind::EnumAccess:
            return false;
        case HIRExpr::Kind::BinOp: {
            auto* e = static_cast<const HIRBinOpExpr*>(expr);
            return exprUsesVar(e->lhs) || exprUsesVar(e->rhs);
        }
        case HIRExpr::Kind::UnaryOp: {
            auto* e = static_cast<const HIRUnaryOpExpr*>(expr);
            return exprUsesVar(e->operand);
        }
        case HIRExpr::Kind::Call: {
            auto* e = static_cast<const HIRCallExpr*>(expr);
            for (uint32_t i = 0; i < e->arg_count; ++i)
                if (exprUsesVar(e->args[i])) return true;
            return false;
        }
        case HIRExpr::Kind::If: {
            auto* e = static_cast<const HIRIfExpr*>(expr);
            return exprUsesVar(e->condition) || exprUsesVar(e->then_branch) ||
                   exprUsesVar(e->else_branch);
        }
        case HIRExpr::Kind::Match: {
            auto* e = static_cast<const HIRMatchExpr*>(expr);
            if (exprUsesVar(e->scrutinee)) return true;
            for (uint32_t i = 0; i < e->arm_count; ++i) {
                if (e->arms[i].guard && exprUsesVar(e->arms[i].guard)) return true;
                if (exprUsesVar(e->arms[i].body)) return true;
            }
            return false;
        }
        case HIRExpr::Kind::Block: {
            auto* e = static_cast<const HIRBlockExpr*>(expr);
            for (uint32_t i = 0; i < e->stmt_count; ++i)
                if (stmtUsesVar(e->stmts[i])) return true;
            return exprUsesVar(e->result);
        }
        case HIRExpr::Kind::Return: {
            auto* e = static_cast<const HIRReturnExpr*>(expr);
            return exprUsesVar(e->value);
        }
        case HIRExpr::Kind::StructLit: {
            auto* e = static_cast<const HIRStructLitExpr*>(expr);
            for (uint32_t i = 0; i < e->field_count; ++i)
                if (exprUsesVar(e->fields[i].value)) return true;
            return false;
        }
        case HIRExpr::Kind::FieldAccess: {
            auto* e = static_cast<const HIRFieldAccessExpr*>(expr);
            return exprUsesVar(e->object);
        }
        case HIRExpr::Kind::UnionVariant: {
            auto* e = static_cast<const HIRUnionVariantExpr*>(expr);
            return exprUsesVar(e->payload);
        }
        case HIRExpr::Kind::AddrOf: {
            auto* e = static_cast<const HIRAddrOfExpr*>(expr);
            return exprUsesVar(e->operand);
        }
        case HIRExpr::Kind::Deref: {
            auto* e = static_cast<const HIRDerefExpr*>(expr);
            return exprUsesVar(e->operand);
        }
    }
    return false;
}

static bool stmtUsesVar(const HIRStmt* stmt) {
    if (!stmt) return false;
    switch (stmt->kind) {
        case HIRStmt::Kind::VarDecl:
            return true;
        case HIRStmt::Kind::Assign:
            return true;
        case HIRStmt::Kind::FieldAssign:
            return true;
        case HIRStmt::Kind::ValDecl:
            return exprUsesVar(static_cast<const HIRValDeclStmt*>(stmt)->init);
        case HIRStmt::Kind::ExprStmt:
            return exprUsesVar(static_cast<const HIRExprStmt*>(stmt)->expr);
        case HIRStmt::Kind::DerefAssign:
            return false; // ptr write, not var mutation
    }
    return false;
}

// ============================================================================
// Helper: check if HIR function body uses pointer writes
// ============================================================================

static bool exprUsesPtrWrite(const HIRExpr* expr);
static bool stmtUsesPtrWrite(const HIRStmt* stmt);

static bool exprUsesPtrWrite(const HIRExpr* expr) {
    if (!expr) return false;
    switch (expr->kind) {
        case HIRExpr::Kind::IntLit:
        case HIRExpr::Kind::FloatLit:
        case HIRExpr::Kind::BoolLit:
        case HIRExpr::Kind::StringLit:
        case HIRExpr::Kind::Ident:
        case HIRExpr::Kind::EnumAccess:
            return false;
        case HIRExpr::Kind::BinOp: {
            auto* e = static_cast<const HIRBinOpExpr*>(expr);
            return exprUsesPtrWrite(e->lhs) || exprUsesPtrWrite(e->rhs);
        }
        case HIRExpr::Kind::UnaryOp: {
            auto* e = static_cast<const HIRUnaryOpExpr*>(expr);
            return exprUsesPtrWrite(e->operand);
        }
        case HIRExpr::Kind::Call: {
            auto* e = static_cast<const HIRCallExpr*>(expr);
            for (uint32_t i = 0; i < e->arg_count; ++i)
                if (exprUsesPtrWrite(e->args[i])) return true;
            return false;
        }
        case HIRExpr::Kind::If: {
            auto* e = static_cast<const HIRIfExpr*>(expr);
            return exprUsesPtrWrite(e->condition) || exprUsesPtrWrite(e->then_branch) ||
                   exprUsesPtrWrite(e->else_branch);
        }
        case HIRExpr::Kind::Match: {
            auto* e = static_cast<const HIRMatchExpr*>(expr);
            if (exprUsesPtrWrite(e->scrutinee)) return true;
            for (uint32_t i = 0; i < e->arm_count; ++i) {
                if (e->arms[i].guard && exprUsesPtrWrite(e->arms[i].guard)) return true;
                if (exprUsesPtrWrite(e->arms[i].body)) return true;
            }
            return false;
        }
        case HIRExpr::Kind::Block: {
            auto* e = static_cast<const HIRBlockExpr*>(expr);
            for (uint32_t i = 0; i < e->stmt_count; ++i)
                if (stmtUsesPtrWrite(e->stmts[i])) return true;
            return exprUsesPtrWrite(e->result);
        }
        case HIRExpr::Kind::Return: {
            auto* e = static_cast<const HIRReturnExpr*>(expr);
            return exprUsesPtrWrite(e->value);
        }
        case HIRExpr::Kind::StructLit: {
            auto* e = static_cast<const HIRStructLitExpr*>(expr);
            for (uint32_t i = 0; i < e->field_count; ++i)
                if (exprUsesPtrWrite(e->fields[i].value)) return true;
            return false;
        }
        case HIRExpr::Kind::FieldAccess: {
            auto* e = static_cast<const HIRFieldAccessExpr*>(expr);
            return exprUsesPtrWrite(e->object);
        }
        case HIRExpr::Kind::UnionVariant: {
            auto* e = static_cast<const HIRUnionVariantExpr*>(expr);
            return exprUsesPtrWrite(e->payload);
        }
        case HIRExpr::Kind::AddrOf: {
            auto* e = static_cast<const HIRAddrOfExpr*>(expr);
            if (e->is_mutable) return true;
            return exprUsesPtrWrite(e->operand);
        }
        case HIRExpr::Kind::Deref: {
            auto* e = static_cast<const HIRDerefExpr*>(expr);
            return exprUsesPtrWrite(e->operand);
        }
    }
    return false;
}

static bool stmtUsesPtrWrite(const HIRStmt* stmt) {
    if (!stmt) return false;
    switch (stmt->kind) {
        case HIRStmt::Kind::DerefAssign:
            return true;
        case HIRStmt::Kind::ValDecl:
            return exprUsesPtrWrite(static_cast<const HIRValDeclStmt*>(stmt)->init);
        case HIRStmt::Kind::VarDecl:
            return exprUsesPtrWrite(static_cast<const HIRVarDeclStmt*>(stmt)->init);
        case HIRStmt::Kind::ExprStmt:
            return exprUsesPtrWrite(static_cast<const HIRExprStmt*>(stmt)->expr);
        case HIRStmt::Kind::Assign:
            return exprUsesPtrWrite(static_cast<const HIRAssignStmt*>(stmt)->value);
        case HIRStmt::Kind::FieldAssign: {
            auto* s = static_cast<const HIRFieldAssignStmt*>(stmt);
            return exprUsesPtrWrite(s->target) || exprUsesPtrWrite(s->value);
        }
    }
    return false;
}

// ============================================================================
// PurityAnalysisPass
// ============================================================================

void PurityAnalysisPass::run(HIRModule& module, CompilationContext& ctx) {
    // Build function map and call graph
    std::unordered_map<std::string_view, HIRFnDecl*> fn_map;
    std::unordered_map<std::string_view, std::unordered_set<std::string_view>> call_graph;

    for (uint32_t i = 0; i < module.fn_count; ++i) {
        auto* fn = module.functions[i];
        fn_map[fn->name] = fn;
        std::unordered_set<std::string_view> callees;
        if (fn->body) collectCallees(fn->body, callees);
        call_graph[fn->name] = std::move(callees);
    }

    // Topological sort (Kahn's on reversed call graph — callees before callers)
    // Reverse graph: callee → callers. In-degree = number of callees (out-degree in call graph).
    std::unordered_map<std::string_view, int> out_degree;
    std::unordered_map<std::string_view, std::vector<std::string_view>> reverse_graph;
    for (auto& [name, _] : fn_map) {
        out_degree[name] = 0;
        reverse_graph[name] = {};
    }
    for (auto& [caller, callees] : call_graph) {
        for (auto& callee : callees) {
            if (callee == caller) continue; // skip self-loops
            if (!fn_map.count(callee)) continue;
            out_degree[caller]++;
            reverse_graph[callee].push_back(caller);
        }
    }

    std::queue<std::string_view> queue;
    for (auto& [name, deg] : out_degree) {
        if (deg == 0) queue.push(name); // leaf functions (call nothing)
    }

    std::vector<std::string_view> order;
    std::unordered_set<std::string_view> visited;
    while (!queue.empty()) {
        auto name = queue.front(); queue.pop();
        order.push_back(name);
        visited.insert(name);
        for (auto& caller : reverse_graph[name]) {
            if (--out_degree[caller] == 0) queue.push(caller);
        }
    }
    // Append cycle members
    for (auto& [name, _] : fn_map) {
        if (!visited.count(name)) order.push_back(name);
    }

    // Purity results
    std::unordered_map<std::string_view, Purity> purity_map;

    // Process in order (leaves first — callees before callers)
    for (auto& name : order) {
        auto* fn = fn_map[name];
        Purity purity = Purity::Pure;

        // Intrinsics → ImpureIo
        if (fn->is_intrinsic) {
            purity = Purity::ImpureIo;
        } else if (fn->body) {
            // Local: var mutation → ImpureMut
            if (exprUsesVar(fn->body)) {
                purity = Purity::ImpureMut;
                ctx.diag.warning(fn->loc,
                    std::string("function '") + std::string(name) +
                    "' uses mutable state (var/assign)");
            }
            // Local: pointer writes → ImpureMem (overrides ImpureMut)
            if (exprUsesPtrWrite(fn->body)) {
                purity = Purity::ImpureMem;
            }
            // Propagate from callees
            for (auto& callee : call_graph[name]) {
                if (!purity_map.count(callee)) continue;
                Purity callee_purity = purity_map[callee];
                if (callee_purity == Purity::ImpureIo) {
                    purity = Purity::ImpureIo;
                } else if (callee_purity == Purity::ImpureMem && purity != Purity::ImpureIo) {
                    purity = Purity::ImpureMem;
                }
                // ImpureMut does NOT propagate
            }
        }

        purity_map[name] = purity;
        fn->purity = static_cast<uint8_t>(purity);
    }
}

// ============================================================================
// TailCallAnalysisPass — helpers
// ============================================================================

// Forward declarations for tail call marking
static void markTailCalls(HIRExpr* expr, bool in_tail_pos,
                          std::string_view fn_name);
static void markTailCallsStmt(HIRStmt* stmt, std::string_view fn_name);

static void markTailCalls(HIRExpr* expr, bool in_tail_pos,
                          std::string_view fn_name) {
    if (!expr) return;
    switch (expr->kind) {
        case HIRExpr::Kind::IntLit:
        case HIRExpr::Kind::FloatLit:
        case HIRExpr::Kind::BoolLit:
        case HIRExpr::Kind::StringLit:
        case HIRExpr::Kind::Ident:
        case HIRExpr::Kind::EnumAccess:
            break;
        case HIRExpr::Kind::BinOp: {
            auto* e = static_cast<HIRBinOpExpr*>(expr);
            markTailCalls(e->lhs, false, fn_name);
            markTailCalls(e->rhs, false, fn_name);
            break;
        }
        case HIRExpr::Kind::UnaryOp: {
            auto* e = static_cast<HIRUnaryOpExpr*>(expr);
            markTailCalls(e->operand, false, fn_name);
            break;
        }
        case HIRExpr::Kind::Call: {
            auto* e = static_cast<HIRCallExpr*>(expr);
            e->is_tail_call = in_tail_pos;
            for (uint32_t i = 0; i < e->arg_count; ++i)
                markTailCalls(e->args[i], false, fn_name);
            break;
        }
        case HIRExpr::Kind::If: {
            auto* e = static_cast<HIRIfExpr*>(expr);
            markTailCalls(e->condition, false, fn_name);
            markTailCalls(e->then_branch, in_tail_pos, fn_name);
            markTailCalls(e->else_branch, in_tail_pos, fn_name);
            break;
        }
        case HIRExpr::Kind::Match: {
            auto* e = static_cast<HIRMatchExpr*>(expr);
            markTailCalls(e->scrutinee, false, fn_name);
            for (uint32_t i = 0; i < e->arm_count; ++i) {
                if (e->arms[i].guard)
                    markTailCalls(e->arms[i].guard, false, fn_name);
                markTailCalls(e->arms[i].body, in_tail_pos, fn_name);
            }
            break;
        }
        case HIRExpr::Kind::Block: {
            auto* e = static_cast<HIRBlockExpr*>(expr);
            for (uint32_t i = 0; i < e->stmt_count; ++i)
                markTailCallsStmt(e->stmts[i], fn_name);
            markTailCalls(e->result, in_tail_pos, fn_name);
            break;
        }
        case HIRExpr::Kind::Return: {
            auto* e = static_cast<HIRReturnExpr*>(expr);
            markTailCalls(e->value, true, fn_name);
            break;
        }
        case HIRExpr::Kind::StructLit: {
            auto* e = static_cast<HIRStructLitExpr*>(expr);
            for (uint32_t i = 0; i < e->field_count; ++i)
                markTailCalls(e->fields[i].value, false, fn_name);
            break;
        }
        case HIRExpr::Kind::FieldAccess: {
            auto* e = static_cast<HIRFieldAccessExpr*>(expr);
            markTailCalls(e->object, false, fn_name);
            break;
        }
        case HIRExpr::Kind::UnionVariant: {
            auto* e = static_cast<HIRUnionVariantExpr*>(expr);
            markTailCalls(e->payload, false, fn_name);
            break;
        }
        case HIRExpr::Kind::AddrOf: {
            auto* e = static_cast<HIRAddrOfExpr*>(expr);
            markTailCalls(e->operand, false, fn_name);
            break;
        }
        case HIRExpr::Kind::Deref: {
            auto* e = static_cast<HIRDerefExpr*>(expr);
            markTailCalls(e->operand, false, fn_name);
            break;
        }
    }
}

static void markTailCallsStmt(HIRStmt* stmt, std::string_view fn_name) {
    if (!stmt) return;
    switch (stmt->kind) {
        case HIRStmt::Kind::ValDecl:
            markTailCalls(static_cast<HIRValDeclStmt*>(stmt)->init, false, fn_name);
            break;
        case HIRStmt::Kind::VarDecl:
            markTailCalls(static_cast<HIRVarDeclStmt*>(stmt)->init, false, fn_name);
            break;
        case HIRStmt::Kind::ExprStmt:
            markTailCalls(static_cast<HIRExprStmt*>(stmt)->expr, false, fn_name);
            break;
        case HIRStmt::Kind::Assign:
            markTailCalls(static_cast<HIRAssignStmt*>(stmt)->value, false, fn_name);
            break;
        case HIRStmt::Kind::FieldAssign: {
            auto* s = static_cast<HIRFieldAssignStmt*>(stmt);
            markTailCalls(s->target, false, fn_name);
            markTailCalls(s->value, false, fn_name);
            break;
        }
        case HIRStmt::Kind::DerefAssign: {
            auto* s = static_cast<HIRDerefAssignStmt*>(stmt);
            markTailCalls(s->target, false, fn_name);
            markTailCalls(s->value, false, fn_name);
            break;
        }
    }
}

// Check if expression contains any call to fn_name
static bool hasCallTo(const HIRExpr* expr, std::string_view fn_name);
static bool hasCallToStmt(const HIRStmt* stmt, std::string_view fn_name);

static bool hasCallTo(const HIRExpr* expr, std::string_view fn_name) {
    if (!expr) return false;
    switch (expr->kind) {
        case HIRExpr::Kind::IntLit:
        case HIRExpr::Kind::FloatLit:
        case HIRExpr::Kind::BoolLit:
        case HIRExpr::Kind::StringLit:
        case HIRExpr::Kind::Ident:
        case HIRExpr::Kind::EnumAccess:
            return false;
        case HIRExpr::Kind::Call: {
            auto* e = static_cast<const HIRCallExpr*>(expr);
            if (e->callee == fn_name) return true;
            for (uint32_t i = 0; i < e->arg_count; ++i)
                if (hasCallTo(e->args[i], fn_name)) return true;
            return false;
        }
        case HIRExpr::Kind::BinOp: {
            auto* e = static_cast<const HIRBinOpExpr*>(expr);
            return hasCallTo(e->lhs, fn_name) || hasCallTo(e->rhs, fn_name);
        }
        case HIRExpr::Kind::UnaryOp: {
            auto* e = static_cast<const HIRUnaryOpExpr*>(expr);
            return hasCallTo(e->operand, fn_name);
        }
        case HIRExpr::Kind::If: {
            auto* e = static_cast<const HIRIfExpr*>(expr);
            return hasCallTo(e->condition, fn_name) ||
                   hasCallTo(e->then_branch, fn_name) ||
                   hasCallTo(e->else_branch, fn_name);
        }
        case HIRExpr::Kind::Match: {
            auto* e = static_cast<const HIRMatchExpr*>(expr);
            if (hasCallTo(e->scrutinee, fn_name)) return true;
            for (uint32_t i = 0; i < e->arm_count; ++i) {
                if (e->arms[i].guard && hasCallTo(e->arms[i].guard, fn_name)) return true;
                if (hasCallTo(e->arms[i].body, fn_name)) return true;
            }
            return false;
        }
        case HIRExpr::Kind::Block: {
            auto* e = static_cast<const HIRBlockExpr*>(expr);
            for (uint32_t i = 0; i < e->stmt_count; ++i)
                if (hasCallToStmt(e->stmts[i], fn_name)) return true;
            return hasCallTo(e->result, fn_name);
        }
        case HIRExpr::Kind::Return: {
            auto* e = static_cast<const HIRReturnExpr*>(expr);
            return hasCallTo(e->value, fn_name);
        }
        case HIRExpr::Kind::StructLit: {
            auto* e = static_cast<const HIRStructLitExpr*>(expr);
            for (uint32_t i = 0; i < e->field_count; ++i)
                if (hasCallTo(e->fields[i].value, fn_name)) return true;
            return false;
        }
        case HIRExpr::Kind::FieldAccess:
            return hasCallTo(static_cast<const HIRFieldAccessExpr*>(expr)->object, fn_name);
        case HIRExpr::Kind::UnionVariant:
            return hasCallTo(static_cast<const HIRUnionVariantExpr*>(expr)->payload, fn_name);
        case HIRExpr::Kind::AddrOf:
            return hasCallTo(static_cast<const HIRAddrOfExpr*>(expr)->operand, fn_name);
        case HIRExpr::Kind::Deref:
            return hasCallTo(static_cast<const HIRDerefExpr*>(expr)->operand, fn_name);
    }
    return false;
}

static bool hasCallToStmt(const HIRStmt* stmt, std::string_view fn_name) {
    if (!stmt) return false;
    switch (stmt->kind) {
        case HIRStmt::Kind::ValDecl:
            return hasCallTo(static_cast<const HIRValDeclStmt*>(stmt)->init, fn_name);
        case HIRStmt::Kind::VarDecl:
            return hasCallTo(static_cast<const HIRVarDeclStmt*>(stmt)->init, fn_name);
        case HIRStmt::Kind::ExprStmt:
            return hasCallTo(static_cast<const HIRExprStmt*>(stmt)->expr, fn_name);
        case HIRStmt::Kind::Assign:
            return hasCallTo(static_cast<const HIRAssignStmt*>(stmt)->value, fn_name);
        case HIRStmt::Kind::FieldAssign: {
            auto* s = static_cast<const HIRFieldAssignStmt*>(stmt);
            return hasCallTo(s->target, fn_name) || hasCallTo(s->value, fn_name);
        }
        case HIRStmt::Kind::DerefAssign: {
            auto* s = static_cast<const HIRDerefAssignStmt*>(stmt);
            return hasCallTo(s->target, fn_name) || hasCallTo(s->value, fn_name);
        }
    }
    return false;
}

// Check if expression has a non-tail recursive call
static bool hasNonTailCall(const HIRExpr* expr, std::string_view fn_name, bool in_tail_pos);
static bool hasNonTailCallStmt(const HIRStmt* stmt, std::string_view fn_name);

static bool hasNonTailCall(const HIRExpr* expr, std::string_view fn_name, bool in_tail_pos) {
    if (!expr) return false;
    switch (expr->kind) {
        case HIRExpr::Kind::IntLit:
        case HIRExpr::Kind::FloatLit:
        case HIRExpr::Kind::BoolLit:
        case HIRExpr::Kind::StringLit:
        case HIRExpr::Kind::Ident:
        case HIRExpr::Kind::EnumAccess:
            return false;
        case HIRExpr::Kind::Call: {
            auto* e = static_cast<const HIRCallExpr*>(expr);
            // Non-tail recursive call found
            if (e->callee == fn_name && !in_tail_pos) return true;
            // Check args (always non-tail)
            for (uint32_t i = 0; i < e->arg_count; ++i)
                if (hasNonTailCall(e->args[i], fn_name, false)) return true;
            return false;
        }
        case HIRExpr::Kind::BinOp: {
            auto* e = static_cast<const HIRBinOpExpr*>(expr);
            return hasNonTailCall(e->lhs, fn_name, false) ||
                   hasNonTailCall(e->rhs, fn_name, false);
        }
        case HIRExpr::Kind::UnaryOp: {
            auto* e = static_cast<const HIRUnaryOpExpr*>(expr);
            return hasNonTailCall(e->operand, fn_name, false);
        }
        case HIRExpr::Kind::If: {
            auto* e = static_cast<const HIRIfExpr*>(expr);
            return hasNonTailCall(e->condition, fn_name, false) ||
                   hasNonTailCall(e->then_branch, fn_name, in_tail_pos) ||
                   hasNonTailCall(e->else_branch, fn_name, in_tail_pos);
        }
        case HIRExpr::Kind::Match: {
            auto* e = static_cast<const HIRMatchExpr*>(expr);
            if (hasNonTailCall(e->scrutinee, fn_name, false)) return true;
            for (uint32_t i = 0; i < e->arm_count; ++i) {
                if (e->arms[i].guard && hasNonTailCall(e->arms[i].guard, fn_name, false))
                    return true;
                if (hasNonTailCall(e->arms[i].body, fn_name, in_tail_pos)) return true;
            }
            return false;
        }
        case HIRExpr::Kind::Block: {
            auto* e = static_cast<const HIRBlockExpr*>(expr);
            for (uint32_t i = 0; i < e->stmt_count; ++i)
                if (hasNonTailCallStmt(e->stmts[i], fn_name)) return true;
            return hasNonTailCall(e->result, fn_name, in_tail_pos);
        }
        case HIRExpr::Kind::Return: {
            auto* e = static_cast<const HIRReturnExpr*>(expr);
            return hasNonTailCall(e->value, fn_name, true);
        }
        case HIRExpr::Kind::StructLit: {
            auto* e = static_cast<const HIRStructLitExpr*>(expr);
            for (uint32_t i = 0; i < e->field_count; ++i)
                if (hasNonTailCall(e->fields[i].value, fn_name, false)) return true;
            return false;
        }
        case HIRExpr::Kind::FieldAccess:
            return hasNonTailCall(static_cast<const HIRFieldAccessExpr*>(expr)->object, fn_name, false);
        case HIRExpr::Kind::UnionVariant:
            return hasNonTailCall(static_cast<const HIRUnionVariantExpr*>(expr)->payload, fn_name, false);
        case HIRExpr::Kind::AddrOf:
            return hasNonTailCall(static_cast<const HIRAddrOfExpr*>(expr)->operand, fn_name, false);
        case HIRExpr::Kind::Deref:
            return hasNonTailCall(static_cast<const HIRDerefExpr*>(expr)->operand, fn_name, false);
    }
    return false;
}

static bool hasNonTailCallStmt(const HIRStmt* stmt, std::string_view fn_name) {
    if (!stmt) return false;
    switch (stmt->kind) {
        case HIRStmt::Kind::ValDecl:
            return hasNonTailCall(static_cast<const HIRValDeclStmt*>(stmt)->init, fn_name, false);
        case HIRStmt::Kind::VarDecl:
            return hasNonTailCall(static_cast<const HIRVarDeclStmt*>(stmt)->init, fn_name, false);
        case HIRStmt::Kind::ExprStmt:
            return hasNonTailCall(static_cast<const HIRExprStmt*>(stmt)->expr, fn_name, false);
        case HIRStmt::Kind::Assign:
            return hasNonTailCall(static_cast<const HIRAssignStmt*>(stmt)->value, fn_name, false);
        case HIRStmt::Kind::FieldAssign: {
            auto* s = static_cast<const HIRFieldAssignStmt*>(stmt);
            return hasNonTailCall(s->target, fn_name, false) ||
                   hasNonTailCall(s->value, fn_name, false);
        }
        case HIRStmt::Kind::DerefAssign: {
            auto* s = static_cast<const HIRDerefAssignStmt*>(stmt);
            return hasNonTailCall(s->target, fn_name, false) ||
                   hasNonTailCall(s->value, fn_name, false);
        }
    }
    return false;
}

// ============================================================================
// TailCallAnalysisPass
// ============================================================================

void TailCallAnalysisPass::run(HIRModule& module, CompilationContext& /*ctx*/) {
    // Build call graph for recursion detection
    std::unordered_map<std::string_view, std::unordered_set<std::string_view>> call_graph;
    std::unordered_set<std::string_view> fn_names;

    for (uint32_t i = 0; i < module.fn_count; ++i) {
        auto* fn = module.functions[i];
        fn_names.insert(fn->name);
        std::unordered_set<std::string_view> callees;
        if (fn->body) collectCallees(fn->body, callees);
        call_graph[fn->name] = std::move(callees);
    }

    for (uint32_t i = 0; i < module.fn_count; ++i) {
        auto* fn = module.functions[i];
        if (!fn->body) continue;

        // Mark tail calls in the body (body is always in tail position)
        markTailCalls(fn->body, true, fn->name);

        // Detect recursion: direct or mutual (DFS)
        bool is_recursive = false;
        {
            // Direct self-recursion
            if (call_graph[fn->name].count(fn->name)) {
                is_recursive = true;
            } else {
                // Mutual recursion: DFS from callees
                std::unordered_set<std::string_view> visited;
                std::vector<std::string_view> stack;
                for (auto& callee : call_graph[fn->name]) {
                    if (fn_names.count(callee) && callee != fn->name)
                        stack.push_back(callee);
                }
                while (!stack.empty() && !is_recursive) {
                    auto cur = stack.back(); stack.pop_back();
                    if (visited.count(cur)) continue;
                    visited.insert(cur);
                    if (cur == fn->name) { is_recursive = true; break; }
                    for (auto& callee : call_graph[cur]) {
                        if (fn_names.count(callee) && !visited.count(callee))
                            stack.push_back(callee);
                    }
                }
            }
        }

        fn->is_recursive = is_recursive;

        // Detect tail recursion: has tail call AND no non-tail call to self
        if (is_recursive) {
            bool has_self_call = hasCallTo(fn->body, fn->name);
            bool has_non_tail = hasNonTailCall(fn->body, fn->name, true);
            fn->is_tail_recursive = has_self_call && !has_non_tail;
        } else {
            fn->is_tail_recursive = false;
        }
    }
}

} // namespace kern
