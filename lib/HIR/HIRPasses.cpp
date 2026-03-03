#include "kern/hir/HIRPasses.h"
#include "kern/support/CompilationContext.h"
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
        case HIRExpr::Kind::Cast: {
            auto* e = static_cast<const HIRCastExpr*>(expr);
            collectCallees(e->operand, out);
            break;
        }
        case HIRExpr::Kind::Loop: {
            auto* e = static_cast<const HIRLoopExpr*>(expr);
            for (uint32_t i = 0; i < e->binding_count; ++i)
                collectCallees(e->bindings[i].init, out);
            collectCallees(e->body, out);
            break;
        }
        case HIRExpr::Kind::Break: {
            auto* e = static_cast<const HIRBreakExpr*>(expr);
            collectCallees(e->value, out);
            break;
        }
        case HIRExpr::Kind::Continue: {
            auto* e = static_cast<const HIRContinueExpr*>(expr);
            for (uint32_t i = 0; i < e->arg_count; ++i)
                collectCallees(e->args[i], out);
            break;
        }
        case HIRExpr::Kind::ArrayLit: {
            auto* e = static_cast<const HIRArrayLitExpr*>(expr);
            for (uint32_t i = 0; i < e->element_count; ++i)
                collectCallees(e->elements[i], out);
            break;
        }
        case HIRExpr::Kind::IndexAccess: {
            auto* e = static_cast<const HIRIndexAccessExpr*>(expr);
            collectCallees(e->array, out);
            collectCallees(e->index, out);
            break;
        }
        case HIRExpr::Kind::InlineAsm:
            break;
        case HIRExpr::Kind::FnRef:
            break;
        case HIRExpr::Kind::CallIndirect: {
            auto* e = static_cast<const HIRCallIndirectExpr*>(expr);
            collectCallees(e->callee, out);
            for (uint32_t i = 0; i < e->arg_count; ++i)
                collectCallees(e->args[i], out);
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
        case HIRStmt::Kind::IndexAssign: {
            auto* s = static_cast<const HIRIndexAssignStmt*>(stmt);
            collectCallees(s->array, out);
            collectCallees(s->index, out);
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
        case HIRExpr::Kind::Cast: {
            auto* e = static_cast<const HIRCastExpr*>(expr);
            return exprUsesVar(e->operand);
        }
        case HIRExpr::Kind::Loop: {
            auto* e = static_cast<const HIRLoopExpr*>(expr);
            for (uint32_t i = 0; i < e->binding_count; ++i)
                if (exprUsesVar(e->bindings[i].init)) return true;
            return exprUsesVar(e->body);
        }
        case HIRExpr::Kind::Break: {
            auto* e = static_cast<const HIRBreakExpr*>(expr);
            return exprUsesVar(e->value);
        }
        case HIRExpr::Kind::Continue: {
            auto* e = static_cast<const HIRContinueExpr*>(expr);
            for (uint32_t i = 0; i < e->arg_count; ++i)
                if (exprUsesVar(e->args[i])) return true;
            return false;
        }
        case HIRExpr::Kind::ArrayLit: {
            auto* e = static_cast<const HIRArrayLitExpr*>(expr);
            for (uint32_t i = 0; i < e->element_count; ++i)
                if (exprUsesVar(e->elements[i])) return true;
            return false;
        }
        case HIRExpr::Kind::IndexAccess: {
            auto* e = static_cast<const HIRIndexAccessExpr*>(expr);
            return exprUsesVar(e->array) || exprUsesVar(e->index);
        }
        case HIRExpr::Kind::InlineAsm:
            return false;
        case HIRExpr::Kind::FnRef:
            return false;
        case HIRExpr::Kind::CallIndirect: {
            auto* e = static_cast<const HIRCallIndirectExpr*>(expr);
            if (exprUsesVar(e->callee)) return true;
            for (uint32_t i = 0; i < e->arg_count; ++i)
                if (exprUsesVar(e->args[i])) return true;
            return false;
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
        case HIRStmt::Kind::IndexAssign:
            return true; // array mutation counts as var mutation
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
            // Any pointer creation (& or &var) requires mem effect
            return true;
        }
        case HIRExpr::Kind::Deref: {
            auto* e = static_cast<const HIRDerefExpr*>(expr);
            return exprUsesPtrWrite(e->operand);
        }
        case HIRExpr::Kind::Cast: {
            auto* e = static_cast<const HIRCastExpr*>(expr);
            return exprUsesPtrWrite(e->operand);
        }
        case HIRExpr::Kind::Loop: {
            auto* e = static_cast<const HIRLoopExpr*>(expr);
            for (uint32_t i = 0; i < e->binding_count; ++i)
                if (exprUsesPtrWrite(e->bindings[i].init)) return true;
            return exprUsesPtrWrite(e->body);
        }
        case HIRExpr::Kind::Break: {
            auto* e = static_cast<const HIRBreakExpr*>(expr);
            return exprUsesPtrWrite(e->value);
        }
        case HIRExpr::Kind::Continue: {
            auto* e = static_cast<const HIRContinueExpr*>(expr);
            for (uint32_t i = 0; i < e->arg_count; ++i)
                if (exprUsesPtrWrite(e->args[i])) return true;
            return false;
        }
        case HIRExpr::Kind::ArrayLit: {
            auto* e = static_cast<const HIRArrayLitExpr*>(expr);
            for (uint32_t i = 0; i < e->element_count; ++i)
                if (exprUsesPtrWrite(e->elements[i])) return true;
            return false;
        }
        case HIRExpr::Kind::IndexAccess: {
            auto* e = static_cast<const HIRIndexAccessExpr*>(expr);
            return exprUsesPtrWrite(e->array) || exprUsesPtrWrite(e->index);
        }
        case HIRExpr::Kind::InlineAsm:
            return false;
        case HIRExpr::Kind::FnRef:
            return false;
        case HIRExpr::Kind::CallIndirect: {
            auto* e = static_cast<const HIRCallIndirectExpr*>(expr);
            if (exprUsesPtrWrite(e->callee)) return true;
            for (uint32_t i = 0; i < e->arg_count; ++i)
                if (exprUsesPtrWrite(e->args[i])) return true;
            return false;
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
        case HIRStmt::Kind::IndexAssign: {
            auto* s = static_cast<const HIRIndexAssignStmt*>(stmt);
            return exprUsesPtrWrite(s->array) || exprUsesPtrWrite(s->index) ||
                   exprUsesPtrWrite(s->value);
        }
    }
    return false;
}

// ============================================================================
// Helper: check if HIR function body contains inline asm (→ ImpureIo)
// ============================================================================

static bool exprUsesInlineAsm(const HIRExpr* expr);
static bool stmtUsesInlineAsm(const HIRStmt* stmt);

static bool exprUsesInlineAsm(const HIRExpr* expr) {
    if (!expr) return false;
    switch (expr->kind) {
        case HIRExpr::Kind::InlineAsm:
            return true;
        case HIRExpr::Kind::IntLit:
        case HIRExpr::Kind::FloatLit:
        case HIRExpr::Kind::BoolLit:
        case HIRExpr::Kind::StringLit:
        case HIRExpr::Kind::Ident:
        case HIRExpr::Kind::EnumAccess:
            return false;
        case HIRExpr::Kind::BinOp: {
            auto* e = static_cast<const HIRBinOpExpr*>(expr);
            return exprUsesInlineAsm(e->lhs) || exprUsesInlineAsm(e->rhs);
        }
        case HIRExpr::Kind::UnaryOp:
            return exprUsesInlineAsm(static_cast<const HIRUnaryOpExpr*>(expr)->operand);
        case HIRExpr::Kind::Call: {
            auto* e = static_cast<const HIRCallExpr*>(expr);
            for (uint32_t i = 0; i < e->arg_count; ++i)
                if (exprUsesInlineAsm(e->args[i])) return true;
            return false;
        }
        case HIRExpr::Kind::If: {
            auto* e = static_cast<const HIRIfExpr*>(expr);
            return exprUsesInlineAsm(e->condition) || exprUsesInlineAsm(e->then_branch) ||
                   exprUsesInlineAsm(e->else_branch);
        }
        case HIRExpr::Kind::Match: {
            auto* e = static_cast<const HIRMatchExpr*>(expr);
            if (exprUsesInlineAsm(e->scrutinee)) return true;
            for (uint32_t i = 0; i < e->arm_count; ++i) {
                if (e->arms[i].guard && exprUsesInlineAsm(e->arms[i].guard)) return true;
                if (exprUsesInlineAsm(e->arms[i].body)) return true;
            }
            return false;
        }
        case HIRExpr::Kind::Block: {
            auto* e = static_cast<const HIRBlockExpr*>(expr);
            for (uint32_t i = 0; i < e->stmt_count; ++i)
                if (stmtUsesInlineAsm(e->stmts[i])) return true;
            return exprUsesInlineAsm(e->result);
        }
        case HIRExpr::Kind::Return:
            return exprUsesInlineAsm(static_cast<const HIRReturnExpr*>(expr)->value);
        case HIRExpr::Kind::StructLit: {
            auto* e = static_cast<const HIRStructLitExpr*>(expr);
            for (uint32_t i = 0; i < e->field_count; ++i)
                if (exprUsesInlineAsm(e->fields[i].value)) return true;
            return false;
        }
        case HIRExpr::Kind::FieldAccess:
            return exprUsesInlineAsm(static_cast<const HIRFieldAccessExpr*>(expr)->object);
        case HIRExpr::Kind::UnionVariant:
            return exprUsesInlineAsm(static_cast<const HIRUnionVariantExpr*>(expr)->payload);
        case HIRExpr::Kind::AddrOf:
            return exprUsesInlineAsm(static_cast<const HIRAddrOfExpr*>(expr)->operand);
        case HIRExpr::Kind::Deref:
            return exprUsesInlineAsm(static_cast<const HIRDerefExpr*>(expr)->operand);
        case HIRExpr::Kind::Cast:
            return exprUsesInlineAsm(static_cast<const HIRCastExpr*>(expr)->operand);
        case HIRExpr::Kind::Loop: {
            auto* e = static_cast<const HIRLoopExpr*>(expr);
            for (uint32_t i = 0; i < e->binding_count; ++i)
                if (exprUsesInlineAsm(e->bindings[i].init)) return true;
            return exprUsesInlineAsm(e->body);
        }
        case HIRExpr::Kind::Break:
            return exprUsesInlineAsm(static_cast<const HIRBreakExpr*>(expr)->value);
        case HIRExpr::Kind::Continue: {
            auto* e = static_cast<const HIRContinueExpr*>(expr);
            for (uint32_t i = 0; i < e->arg_count; ++i)
                if (exprUsesInlineAsm(e->args[i])) return true;
            return false;
        }
        case HIRExpr::Kind::ArrayLit: {
            auto* e = static_cast<const HIRArrayLitExpr*>(expr);
            for (uint32_t i = 0; i < e->element_count; ++i)
                if (exprUsesInlineAsm(e->elements[i])) return true;
            return false;
        }
        case HIRExpr::Kind::IndexAccess: {
            auto* e = static_cast<const HIRIndexAccessExpr*>(expr);
            return exprUsesInlineAsm(e->array) || exprUsesInlineAsm(e->index);
        }
        case HIRExpr::Kind::FnRef:
            return false;
        case HIRExpr::Kind::CallIndirect: {
            auto* e = static_cast<const HIRCallIndirectExpr*>(expr);
            if (exprUsesInlineAsm(e->callee)) return true;
            for (uint32_t i = 0; i < e->arg_count; ++i)
                if (exprUsesInlineAsm(e->args[i])) return true;
            return false;
        }
    }
    return false;
}

static bool stmtUsesInlineAsm(const HIRStmt* stmt) {
    if (!stmt) return false;
    switch (stmt->kind) {
        case HIRStmt::Kind::ValDecl:
            return exprUsesInlineAsm(static_cast<const HIRValDeclStmt*>(stmt)->init);
        case HIRStmt::Kind::VarDecl:
            return exprUsesInlineAsm(static_cast<const HIRVarDeclStmt*>(stmt)->init);
        case HIRStmt::Kind::ExprStmt:
            return exprUsesInlineAsm(static_cast<const HIRExprStmt*>(stmt)->expr);
        case HIRStmt::Kind::Assign:
            return exprUsesInlineAsm(static_cast<const HIRAssignStmt*>(stmt)->value);
        case HIRStmt::Kind::FieldAssign: {
            auto* s = static_cast<const HIRFieldAssignStmt*>(stmt);
            return exprUsesInlineAsm(s->target) || exprUsesInlineAsm(s->value);
        }
        case HIRStmt::Kind::DerefAssign: {
            auto* s = static_cast<const HIRDerefAssignStmt*>(stmt);
            return exprUsesInlineAsm(s->target) || exprUsesInlineAsm(s->value);
        }
        case HIRStmt::Kind::IndexAssign: {
            auto* s = static_cast<const HIRIndexAssignStmt*>(stmt);
            return exprUsesInlineAsm(s->array) || exprUsesInlineAsm(s->index) ||
                   exprUsesInlineAsm(s->value);
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
            // Local: inline asm → ImpureIo (overrides all)
            if (exprUsesInlineAsm(fn->body)) {
                purity = Purity::ImpureIo;
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
// EffectAnalysisPass
// ============================================================================

// Helper: check if expression calls an atomic intrinsic
static bool exprUsesAtomicIntrinsic(const HIRExpr* expr);
static bool stmtUsesAtomicIntrinsic(const HIRStmt* stmt);

static bool isAtomicIntrinsicName(std::string_view name) {
    return name == "atomic_load" || name == "atomic_store" ||
           name == "atomic_cas" || name == "atomic_fetch_add" ||
           name == "atomic_fetch_sub" || name == "atomic_fetch_and" ||
           name == "atomic_fetch_or" || name == "atomic_fetch_xor" ||
           name == "atomic_xchg" ||
           name == "mfence" || name == "sfence" || name == "lfence" ||
           name == "compiler_barrier";
}

static bool isIoIntrinsicName(std::string_view name) {
    return name == "volatile_read" || name == "volatile_write" ||
           name == "inb" || name == "inw" || name == "inl" ||
           name == "outb" || name == "outw" || name == "outl" ||
           name == "rdtsc" || name == "rdmsr" || name == "wrmsr" ||
           name == "cpuid_eax" || name == "cpuid_ebx" ||
           name == "cpuid_ecx" || name == "cpuid_edx" ||
           name == "cpuid_sub" ||
           name == "cli" || name == "sti" || name == "hlt" ||
           name == "pause" || name == "swapgs" || name == "wbinvd" ||
           name == "stac" || name == "clac" ||
           name == "sysretq" || name == "sysenter" || name == "sysexit" ||
           name == "read_cr0" || name == "read_cr2" ||
           name == "read_cr3" || name == "read_cr4" ||
           name == "write_cr0" || name == "write_cr3" || name == "write_cr4" ||
           name == "invlpg" || name == "lgdt" || name == "lidt" || name == "ltr" ||
           name == "memcpy" || name == "memset" ||
           name == "fxsave" || name == "fxrstor" ||
           name == "xsave" || name == "xrstor" ||
           name == "read_dr0" || name == "read_dr1" ||
           name == "read_dr2" || name == "read_dr3" ||
           name == "read_dr6" || name == "read_dr7" ||
           name == "write_dr0" || name == "write_dr1" ||
           name == "write_dr2" || name == "write_dr3" ||
           name == "write_dr6" || name == "write_dr7" ||
           name == "rdfsbase" || name == "wrfsbase" ||
           name == "rdgsbase" || name == "wrgsbase" ||
           name == "clflush" || name == "clflushopt" || name == "clwb" ||
           name == "lldt" || name == "sldt" ||
           name == "lmsw" || name == "smsw" ||
           name == "rdpmc";
}

static bool exprUsesAtomicIntrinsic(const HIRExpr* expr) {
    if (!expr) return false;
    switch (expr->kind) {
        case HIRExpr::Kind::Call: {
            auto* e = static_cast<const HIRCallExpr*>(expr);
            if (isAtomicIntrinsicName(e->callee)) return true;
            for (uint32_t i = 0; i < e->arg_count; ++i)
                if (exprUsesAtomicIntrinsic(e->args[i])) return true;
            return false;
        }
        case HIRExpr::Kind::Block: {
            auto* e = static_cast<const HIRBlockExpr*>(expr);
            for (uint32_t i = 0; i < e->stmt_count; ++i)
                if (stmtUsesAtomicIntrinsic(e->stmts[i])) return true;
            return exprUsesAtomicIntrinsic(e->result);
        }
        case HIRExpr::Kind::If: {
            auto* e = static_cast<const HIRIfExpr*>(expr);
            return exprUsesAtomicIntrinsic(e->condition) ||
                   exprUsesAtomicIntrinsic(e->then_branch) ||
                   exprUsesAtomicIntrinsic(e->else_branch);
        }
        case HIRExpr::Kind::Match: {
            auto* e = static_cast<const HIRMatchExpr*>(expr);
            if (exprUsesAtomicIntrinsic(e->scrutinee)) return true;
            for (uint32_t i = 0; i < e->arm_count; ++i) {
                if (e->arms[i].guard && exprUsesAtomicIntrinsic(e->arms[i].guard)) return true;
                if (exprUsesAtomicIntrinsic(e->arms[i].body)) return true;
            }
            return false;
        }
        case HIRExpr::Kind::Return:
            return exprUsesAtomicIntrinsic(static_cast<const HIRReturnExpr*>(expr)->value);
        case HIRExpr::Kind::Loop: {
            auto* e = static_cast<const HIRLoopExpr*>(expr);
            for (uint32_t i = 0; i < e->binding_count; ++i)
                if (exprUsesAtomicIntrinsic(e->bindings[i].init)) return true;
            return exprUsesAtomicIntrinsic(e->body);
        }
        case HIRExpr::Kind::BinOp: {
            auto* e = static_cast<const HIRBinOpExpr*>(expr);
            return exprUsesAtomicIntrinsic(e->lhs) || exprUsesAtomicIntrinsic(e->rhs);
        }
        case HIRExpr::Kind::UnaryOp:
            return exprUsesAtomicIntrinsic(static_cast<const HIRUnaryOpExpr*>(expr)->operand);
        case HIRExpr::Kind::CallIndirect: {
            auto* e = static_cast<const HIRCallIndirectExpr*>(expr);
            if (exprUsesAtomicIntrinsic(e->callee)) return true;
            for (uint32_t i = 0; i < e->arg_count; ++i)
                if (exprUsesAtomicIntrinsic(e->args[i])) return true;
            return false;
        }
        default:
            return false;
    }
}

static bool stmtUsesAtomicIntrinsic(const HIRStmt* stmt) {
    if (!stmt) return false;
    switch (stmt->kind) {
        case HIRStmt::Kind::ValDecl:
            return exprUsesAtomicIntrinsic(static_cast<const HIRValDeclStmt*>(stmt)->init);
        case HIRStmt::Kind::VarDecl:
            return exprUsesAtomicIntrinsic(static_cast<const HIRVarDeclStmt*>(stmt)->init);
        case HIRStmt::Kind::ExprStmt:
            return exprUsesAtomicIntrinsic(static_cast<const HIRExprStmt*>(stmt)->expr);
        case HIRStmt::Kind::Assign:
            return exprUsesAtomicIntrinsic(static_cast<const HIRAssignStmt*>(stmt)->value);
        case HIRStmt::Kind::FieldAssign: {
            auto* s = static_cast<const HIRFieldAssignStmt*>(stmt);
            return exprUsesAtomicIntrinsic(s->target) || exprUsesAtomicIntrinsic(s->value);
        }
        case HIRStmt::Kind::DerefAssign: {
            auto* s = static_cast<const HIRDerefAssignStmt*>(stmt);
            return exprUsesAtomicIntrinsic(s->target) || exprUsesAtomicIntrinsic(s->value);
        }
        case HIRStmt::Kind::IndexAssign: {
            auto* s = static_cast<const HIRIndexAssignStmt*>(stmt);
            return exprUsesAtomicIntrinsic(s->array) || exprUsesAtomicIntrinsic(s->index) ||
                   exprUsesAtomicIntrinsic(s->value);
        }
    }
    return false;
}

// Helper: check if expression calls a volatile/IO intrinsic directly
static bool exprUsesIoIntrinsic(const HIRExpr* expr);
static bool stmtUsesIoIntrinsic(const HIRStmt* stmt);

static bool exprUsesIoIntrinsic(const HIRExpr* expr) {
    if (!expr) return false;
    switch (expr->kind) {
        case HIRExpr::Kind::Call: {
            auto* e = static_cast<const HIRCallExpr*>(expr);
            if (isIoIntrinsicName(e->callee)) return true;
            for (uint32_t i = 0; i < e->arg_count; ++i)
                if (exprUsesIoIntrinsic(e->args[i])) return true;
            return false;
        }
        case HIRExpr::Kind::Block: {
            auto* e = static_cast<const HIRBlockExpr*>(expr);
            for (uint32_t i = 0; i < e->stmt_count; ++i)
                if (stmtUsesIoIntrinsic(e->stmts[i])) return true;
            return exprUsesIoIntrinsic(e->result);
        }
        case HIRExpr::Kind::If: {
            auto* e = static_cast<const HIRIfExpr*>(expr);
            return exprUsesIoIntrinsic(e->condition) ||
                   exprUsesIoIntrinsic(e->then_branch) ||
                   exprUsesIoIntrinsic(e->else_branch);
        }
        case HIRExpr::Kind::Match: {
            auto* e = static_cast<const HIRMatchExpr*>(expr);
            if (exprUsesIoIntrinsic(e->scrutinee)) return true;
            for (uint32_t i = 0; i < e->arm_count; ++i) {
                if (e->arms[i].guard && exprUsesIoIntrinsic(e->arms[i].guard)) return true;
                if (exprUsesIoIntrinsic(e->arms[i].body)) return true;
            }
            return false;
        }
        case HIRExpr::Kind::Return:
            return exprUsesIoIntrinsic(static_cast<const HIRReturnExpr*>(expr)->value);
        case HIRExpr::Kind::Loop: {
            auto* e = static_cast<const HIRLoopExpr*>(expr);
            for (uint32_t i = 0; i < e->binding_count; ++i)
                if (exprUsesIoIntrinsic(e->bindings[i].init)) return true;
            return exprUsesIoIntrinsic(e->body);
        }
        case HIRExpr::Kind::BinOp: {
            auto* e = static_cast<const HIRBinOpExpr*>(expr);
            return exprUsesIoIntrinsic(e->lhs) || exprUsesIoIntrinsic(e->rhs);
        }
        case HIRExpr::Kind::UnaryOp:
            return exprUsesIoIntrinsic(static_cast<const HIRUnaryOpExpr*>(expr)->operand);
        case HIRExpr::Kind::CallIndirect: {
            auto* e = static_cast<const HIRCallIndirectExpr*>(expr);
            if (exprUsesIoIntrinsic(e->callee)) return true;
            for (uint32_t i = 0; i < e->arg_count; ++i)
                if (exprUsesIoIntrinsic(e->args[i])) return true;
            return false;
        }
        default:
            return false;
    }
}

static bool stmtUsesIoIntrinsic(const HIRStmt* stmt) {
    if (!stmt) return false;
    switch (stmt->kind) {
        case HIRStmt::Kind::ValDecl:
            return exprUsesIoIntrinsic(static_cast<const HIRValDeclStmt*>(stmt)->init);
        case HIRStmt::Kind::VarDecl:
            return exprUsesIoIntrinsic(static_cast<const HIRVarDeclStmt*>(stmt)->init);
        case HIRStmt::Kind::ExprStmt:
            return exprUsesIoIntrinsic(static_cast<const HIRExprStmt*>(stmt)->expr);
        case HIRStmt::Kind::Assign:
            return exprUsesIoIntrinsic(static_cast<const HIRAssignStmt*>(stmt)->value);
        case HIRStmt::Kind::FieldAssign: {
            auto* s = static_cast<const HIRFieldAssignStmt*>(stmt);
            return exprUsesIoIntrinsic(s->target) || exprUsesIoIntrinsic(s->value);
        }
        case HIRStmt::Kind::DerefAssign: {
            auto* s = static_cast<const HIRDerefAssignStmt*>(stmt);
            return exprUsesIoIntrinsic(s->target) || exprUsesIoIntrinsic(s->value);
        }
        case HIRStmt::Kind::IndexAssign: {
            auto* s = static_cast<const HIRIndexAssignStmt*>(stmt);
            return exprUsesIoIntrinsic(s->array) || exprUsesIoIntrinsic(s->index) ||
                   exprUsesIoIntrinsic(s->value);
        }
    }
    return false;
}

// Helper: check if expression contains an indirect call (HOF)
static bool exprUsesCallIndirect(const HIRExpr* expr);
static bool stmtUsesCallIndirect(const HIRStmt* stmt);

static bool exprUsesCallIndirect(const HIRExpr* expr) {
    if (!expr) return false;
    switch (expr->kind) {
        case HIRExpr::Kind::CallIndirect:
            return true;
        case HIRExpr::Kind::Block: {
            auto* e = static_cast<const HIRBlockExpr*>(expr);
            for (uint32_t i = 0; i < e->stmt_count; ++i)
                if (stmtUsesCallIndirect(e->stmts[i])) return true;
            return exprUsesCallIndirect(e->result);
        }
        case HIRExpr::Kind::If: {
            auto* e = static_cast<const HIRIfExpr*>(expr);
            return exprUsesCallIndirect(e->condition) ||
                   exprUsesCallIndirect(e->then_branch) ||
                   exprUsesCallIndirect(e->else_branch);
        }
        case HIRExpr::Kind::Match: {
            auto* e = static_cast<const HIRMatchExpr*>(expr);
            if (exprUsesCallIndirect(e->scrutinee)) return true;
            for (uint32_t i = 0; i < e->arm_count; ++i) {
                if (exprUsesCallIndirect(e->arms[i].body)) return true;
            }
            return false;
        }
        case HIRExpr::Kind::Return:
            return exprUsesCallIndirect(static_cast<const HIRReturnExpr*>(expr)->value);
        case HIRExpr::Kind::Loop: {
            auto* e = static_cast<const HIRLoopExpr*>(expr);
            for (uint32_t i = 0; i < e->binding_count; ++i)
                if (exprUsesCallIndirect(e->bindings[i].init)) return true;
            return exprUsesCallIndirect(e->body);
        }
        case HIRExpr::Kind::BinOp: {
            auto* e = static_cast<const HIRBinOpExpr*>(expr);
            return exprUsesCallIndirect(e->lhs) || exprUsesCallIndirect(e->rhs);
        }
        case HIRExpr::Kind::UnaryOp:
            return exprUsesCallIndirect(static_cast<const HIRUnaryOpExpr*>(expr)->operand);
        case HIRExpr::Kind::Call: {
            auto* e = static_cast<const HIRCallExpr*>(expr);
            for (uint32_t i = 0; i < e->arg_count; ++i)
                if (exprUsesCallIndirect(e->args[i])) return true;
            return false;
        }
        default:
            return false;
    }
}

static bool stmtUsesCallIndirect(const HIRStmt* stmt) {
    if (!stmt) return false;
    switch (stmt->kind) {
        case HIRStmt::Kind::ValDecl:
            return exprUsesCallIndirect(static_cast<const HIRValDeclStmt*>(stmt)->init);
        case HIRStmt::Kind::VarDecl:
            return exprUsesCallIndirect(static_cast<const HIRVarDeclStmt*>(stmt)->init);
        case HIRStmt::Kind::ExprStmt:
            return exprUsesCallIndirect(static_cast<const HIRExprStmt*>(stmt)->expr);
        case HIRStmt::Kind::Assign:
            return exprUsesCallIndirect(static_cast<const HIRAssignStmt*>(stmt)->value);
        case HIRStmt::Kind::FieldAssign: {
            auto* s = static_cast<const HIRFieldAssignStmt*>(stmt);
            return exprUsesCallIndirect(s->target) || exprUsesCallIndirect(s->value);
        }
        case HIRStmt::Kind::DerefAssign: {
            auto* s = static_cast<const HIRDerefAssignStmt*>(stmt);
            return exprUsesCallIndirect(s->target) || exprUsesCallIndirect(s->value);
        }
        case HIRStmt::Kind::IndexAssign: {
            auto* s = static_cast<const HIRIndexAssignStmt*>(stmt);
            return exprUsesCallIndirect(s->array) || exprUsesCallIndirect(s->index) ||
                   exprUsesCallIndirect(s->value);
        }
    }
    return false;
}

void EffectAnalysisPass::run(HIRModule& module, CompilationContext& ctx) {
    // Build function map and call graph (same as PurityAnalysisPass)
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
    std::unordered_map<std::string_view, int> out_degree;
    std::unordered_map<std::string_view, std::vector<std::string_view>> reverse_graph;
    for (auto& [name, _] : fn_map) {
        out_degree[name] = 0;
        reverse_graph[name] = {};
    }
    for (auto& [caller, callees] : call_graph) {
        for (auto& callee : callees) {
            if (callee == caller) continue;
            if (!fn_map.count(callee)) continue;
            out_degree[caller]++;
            reverse_graph[callee].push_back(caller);
        }
    }

    std::queue<std::string_view> queue;
    for (auto& [name, deg] : out_degree) {
        if (deg == 0) queue.push(name);
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
    for (auto& [name, _] : fn_map) {
        if (!visited.count(name)) order.push_back(name);
    }

    // Effect inference results
    std::unordered_map<std::string_view, EffectSet> effect_map;

    // Process in order (leaves first — callees before callers)
    for (auto& name : order) {
        auto* fn = fn_map[name];
        EffectSet effects = EFFECT_NONE;

        // Intrinsic functions (= intrinsic): default to IO
        if (fn->is_intrinsic) {
            effects = addEffect(effects, Effect::IO);
        } else if (fn->body) {
            // Local: var mutation → Mut
            if (exprUsesVar(fn->body)) {
                effects = addEffect(effects, Effect::Mut);
            }
            // Local: pointer writes → Mem
            if (exprUsesPtrWrite(fn->body)) {
                effects = addEffect(effects, Effect::Mem);
            }
            // Local: inline asm → IO
            if (exprUsesInlineAsm(fn->body)) {
                effects = addEffect(effects, Effect::IO);
            }
            // Local: volatile_read/write → IO
            if (exprUsesIoIntrinsic(fn->body)) {
                effects = addEffect(effects, Effect::IO);
            }
            // Local: atomic intrinsics → Atomic
            if (exprUsesAtomicIntrinsic(fn->body)) {
                effects = addEffect(effects, Effect::Atomic);
            }
            // Conservative HOF: indirect calls (fn pointers) may have any effect
            if (exprUsesCallIndirect(fn->body)) {
                effects = unionEffects(effects, EFFECT_ALL);
            }
            // Propagate from callees (all effects propagate, unlike Purity where Mut was local)
            for (auto& callee : call_graph[name]) {
                if (!effect_map.count(callee)) continue;
                effects = unionEffects(effects, effect_map[callee]);
            }
        }

        effect_map[name] = effects;
        fn->inferred_effects = effects;

        // Validate: if function has effect annotation, check declared covers inferred
        if (fn->has_effect_annotation &&
            !effectSubset(fn->inferred_effects, fn->declared_effects)) {
            EffectSet missing = fn->inferred_effects & ~fn->declared_effects;
            ctx.diag.error(fn->loc,
                std::string("function '") + std::string(name) +
                "' has undeclared effects: " + effectSetString(missing) +
                " (declared: " + effectSetString(fn->declared_effects) + ")");
        }
    }

    // Call-site enforcement: caller must have callee's effects
    for (auto& [name, callees] : call_graph) {
        auto* fn = fn_map[name];
        // Use declared effects if annotated, otherwise inferred
        EffectSet caller_effects = fn->has_effect_annotation
            ? fn->declared_effects : fn->inferred_effects;

        for (auto& callee_name : callees) {
            EffectSet callee_effects;
            if (fn_map.count(callee_name)) {
                auto* callee_fn = fn_map[callee_name];
                callee_effects = callee_fn->has_effect_annotation
                    ? callee_fn->declared_effects : callee_fn->inferred_effects;
            } else if (effect_map.count(callee_name)) {
                callee_effects = effect_map[callee_name];
            } else {
                continue;
            }

            if (!effectSubset(callee_effects, caller_effects)) {
                EffectSet missing = callee_effects & ~caller_effects;
                ctx.diag.error(fn->loc,
                    std::string("function '") + std::string(name) +
                    "' calls '" + std::string(callee_name) +
                    "' which requires effects: " + effectSetString(callee_effects) +
                    ", but caller has: " + effectSetString(caller_effects) +
                    " (missing: " + effectSetString(missing) + ")");
            }
        }
    }
}

// ============================================================================
// OwnershipCheckPass
// ============================================================================

// Track which variables are 'own' params and detect use-after-move
struct OwnershipState {
    std::unordered_set<std::string_view> own_vars;     // variables with Own passing mode
    std::unordered_set<std::string_view> moved_vars;   // variables that have been moved
    CompilationContext* ctx = nullptr;

    void markMoved(std::string_view name, SourceLocation) {
        if (own_vars.count(name)) {
            moved_vars.insert(name);
        }
    }

    void checkUse(std::string_view name, SourceLocation loc) {
        if (moved_vars.count(name)) {
            ctx->diag.error(loc, std::string("use of moved value '") +
                            std::string(name) + "'");
        }
    }
};

static void checkOwnershipExpr(const HIRExpr* expr, OwnershipState& state);
static void checkOwnershipStmt(const HIRStmt* stmt, OwnershipState& state);

static void checkOwnershipExpr(const HIRExpr* expr, OwnershipState& state) {
    if (!expr) return;
    switch (expr->kind) {
        case HIRExpr::Kind::Ident: {
            auto* e = static_cast<const HIRIdentExpr*>(expr);
            state.checkUse(e->name, e->loc);
            break;
        }
        case HIRExpr::Kind::Call: {
            auto* e = static_cast<const HIRCallExpr*>(expr);
            for (uint32_t i = 0; i < e->arg_count; ++i) {
                checkOwnershipExpr(e->args[i], state);
                // If an own variable is passed as call arg, mark it moved
                if (e->args[i] && e->args[i]->kind == HIRExpr::Kind::Ident) {
                    auto* id = static_cast<const HIRIdentExpr*>(e->args[i]);
                    state.markMoved(id->name, e->loc);
                }
            }
            break;
        }
        case HIRExpr::Kind::CallIndirect: {
            auto* e = static_cast<const HIRCallIndirectExpr*>(expr);
            checkOwnershipExpr(e->callee, state);
            for (uint32_t i = 0; i < e->arg_count; ++i) {
                checkOwnershipExpr(e->args[i], state);
                if (e->args[i] && e->args[i]->kind == HIRExpr::Kind::Ident) {
                    auto* id = static_cast<const HIRIdentExpr*>(e->args[i]);
                    state.markMoved(id->name, e->loc);
                }
            }
            break;
        }
        case HIRExpr::Kind::BinOp: {
            auto* e = static_cast<const HIRBinOpExpr*>(expr);
            checkOwnershipExpr(e->lhs, state);
            checkOwnershipExpr(e->rhs, state);
            break;
        }
        case HIRExpr::Kind::UnaryOp:
            checkOwnershipExpr(static_cast<const HIRUnaryOpExpr*>(expr)->operand, state);
            break;
        case HIRExpr::Kind::If: {
            auto* e = static_cast<const HIRIfExpr*>(expr);
            checkOwnershipExpr(e->condition, state);
            // Fork state for each branch and merge
            OwnershipState then_state = state;
            OwnershipState else_state = state;
            checkOwnershipExpr(e->then_branch, then_state);
            checkOwnershipExpr(e->else_branch, else_state);
            // Merge: both moved → definite, one moved → warning
            for (auto& var : state.own_vars) {
                bool in_then = then_state.moved_vars.count(var) > 0;
                bool in_else = else_state.moved_vars.count(var) > 0;
                if (in_then && in_else) {
                    state.moved_vars.insert(var);
                } else if (in_then || in_else) {
                    state.ctx->diag.warning(expr->loc,
                        std::string("value '") + std::string(var) +
                        "' may be moved in one branch but not another");
                    state.moved_vars.insert(var);
                }
            }
            break;
        }
        case HIRExpr::Kind::Match: {
            auto* e = static_cast<const HIRMatchExpr*>(expr);
            checkOwnershipExpr(e->scrutinee, state);
            // Fork state per arm and merge
            std::vector<OwnershipState> arm_states;
            for (uint32_t i = 0; i < e->arm_count; ++i) {
                arm_states.push_back(state);
                if (e->arms[i].guard) checkOwnershipExpr(e->arms[i].guard, arm_states.back());
                checkOwnershipExpr(e->arms[i].body, arm_states.back());
            }
            // Merge: count how many arms moved each own var
            for (auto& var : state.own_vars) {
                uint32_t move_count = 0;
                for (auto& arm_st : arm_states) {
                    if (arm_st.moved_vars.count(var)) move_count++;
                }
                if (move_count == arm_states.size() && move_count > 0) {
                    state.moved_vars.insert(var);
                } else if (move_count > 0) {
                    state.ctx->diag.warning(expr->loc,
                        std::string("value '") + std::string(var) +
                        "' may be moved in some match arms but not all");
                    state.moved_vars.insert(var);
                }
            }
            break;
        }
        case HIRExpr::Kind::Block: {
            auto* e = static_cast<const HIRBlockExpr*>(expr);
            for (uint32_t i = 0; i < e->stmt_count; ++i)
                checkOwnershipStmt(e->stmts[i], state);
            checkOwnershipExpr(e->result, state);
            break;
        }
        case HIRExpr::Kind::Return:
            checkOwnershipExpr(static_cast<const HIRReturnExpr*>(expr)->value, state);
            break;
        case HIRExpr::Kind::StructLit: {
            auto* e = static_cast<const HIRStructLitExpr*>(expr);
            for (uint32_t i = 0; i < e->field_count; ++i)
                checkOwnershipExpr(e->fields[i].value, state);
            break;
        }
        case HIRExpr::Kind::FieldAccess:
            checkOwnershipExpr(static_cast<const HIRFieldAccessExpr*>(expr)->object, state);
            break;
        case HIRExpr::Kind::UnionVariant:
            checkOwnershipExpr(static_cast<const HIRUnionVariantExpr*>(expr)->payload, state);
            break;
        case HIRExpr::Kind::Cast:
            checkOwnershipExpr(static_cast<const HIRCastExpr*>(expr)->operand, state);
            break;
        case HIRExpr::Kind::AddrOf:
            checkOwnershipExpr(static_cast<const HIRAddrOfExpr*>(expr)->operand, state);
            break;
        case HIRExpr::Kind::Deref:
            checkOwnershipExpr(static_cast<const HIRDerefExpr*>(expr)->operand, state);
            break;
        case HIRExpr::Kind::Loop: {
            auto* e = static_cast<const HIRLoopExpr*>(expr);
            for (uint32_t i = 0; i < e->binding_count; ++i)
                checkOwnershipExpr(e->bindings[i].init, state);
            checkOwnershipExpr(e->body, state);
            break;
        }
        case HIRExpr::Kind::Break:
            checkOwnershipExpr(static_cast<const HIRBreakExpr*>(expr)->value, state);
            break;
        case HIRExpr::Kind::Continue: {
            auto* e = static_cast<const HIRContinueExpr*>(expr);
            for (uint32_t i = 0; i < e->arg_count; ++i)
                checkOwnershipExpr(e->args[i], state);
            break;
        }
        case HIRExpr::Kind::ArrayLit: {
            auto* e = static_cast<const HIRArrayLitExpr*>(expr);
            for (uint32_t i = 0; i < e->element_count; ++i)
                checkOwnershipExpr(e->elements[i], state);
            break;
        }
        case HIRExpr::Kind::IndexAccess: {
            auto* e = static_cast<const HIRIndexAccessExpr*>(expr);
            checkOwnershipExpr(e->array, state);
            checkOwnershipExpr(e->index, state);
            break;
        }
        default:
            break;
    }
}

static void checkOwnershipStmt(const HIRStmt* stmt, OwnershipState& state) {
    if (!stmt) return;
    switch (stmt->kind) {
        case HIRStmt::Kind::ValDecl: {
            auto* s = static_cast<const HIRValDeclStmt*>(stmt);
            checkOwnershipExpr(s->init, state);
            // If init is an own variable, it's being moved
            if (s->init && s->init->kind == HIRExpr::Kind::Ident) {
                auto* id = static_cast<const HIRIdentExpr*>(s->init);
                state.markMoved(id->name, s->loc);
            }
            break;
        }
        case HIRStmt::Kind::VarDecl: {
            auto* s = static_cast<const HIRVarDeclStmt*>(stmt);
            checkOwnershipExpr(s->init, state);
            break;
        }
        case HIRStmt::Kind::ExprStmt:
            checkOwnershipExpr(static_cast<const HIRExprStmt*>(stmt)->expr, state);
            break;
        case HIRStmt::Kind::Assign: {
            auto* s = static_cast<const HIRAssignStmt*>(stmt);
            checkOwnershipExpr(s->value, state);
            break;
        }
        case HIRStmt::Kind::FieldAssign: {
            auto* s = static_cast<const HIRFieldAssignStmt*>(stmt);
            checkOwnershipExpr(s->target, state);
            checkOwnershipExpr(s->value, state);
            break;
        }
        case HIRStmt::Kind::DerefAssign: {
            auto* s = static_cast<const HIRDerefAssignStmt*>(stmt);
            checkOwnershipExpr(s->target, state);
            checkOwnershipExpr(s->value, state);
            break;
        }
        case HIRStmt::Kind::IndexAssign: {
            auto* s = static_cast<const HIRIndexAssignStmt*>(stmt);
            checkOwnershipExpr(s->array, state);
            checkOwnershipExpr(s->index, state);
            checkOwnershipExpr(s->value, state);
            break;
        }
    }
}

void OwnershipCheckPass::run(HIRModule& module, CompilationContext& ctx) {
    for (uint32_t i = 0; i < module.fn_count; ++i) {
        auto* fn = module.functions[i];
        if (!fn->body) continue;

        OwnershipState state;
        state.ctx = &ctx;

        // Register own parameters
        for (uint32_t j = 0; j < fn->param_count; ++j) {
            if (fn->params[j].passing_mode == 2) {  // Own
                state.own_vars.insert(fn->params[j].name);
            }
        }

        // Walk function body
        checkOwnershipExpr(fn->body, state);
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
        case HIRExpr::Kind::Cast: {
            auto* e = static_cast<HIRCastExpr*>(expr);
            markTailCalls(e->operand, false, fn_name);
            break;
        }
        case HIRExpr::Kind::Loop: {
            auto* e = static_cast<HIRLoopExpr*>(expr);
            for (uint32_t i = 0; i < e->binding_count; ++i)
                markTailCalls(e->bindings[i].init, false, fn_name);
            markTailCalls(e->body, in_tail_pos, fn_name);
            break;
        }
        case HIRExpr::Kind::Break: {
            auto* e = static_cast<HIRBreakExpr*>(expr);
            // break value exits to loop's continuation, NOT tail position
            markTailCalls(e->value, false, fn_name);
            break;
        }
        case HIRExpr::Kind::Continue: {
            auto* e = static_cast<HIRContinueExpr*>(expr);
            for (uint32_t i = 0; i < e->arg_count; ++i)
                markTailCalls(e->args[i], false, fn_name);
            break;
        }
        case HIRExpr::Kind::ArrayLit: {
            auto* e = static_cast<HIRArrayLitExpr*>(expr);
            for (uint32_t i = 0; i < e->element_count; ++i)
                markTailCalls(e->elements[i], false, fn_name);
            break;
        }
        case HIRExpr::Kind::IndexAccess: {
            auto* e = static_cast<HIRIndexAccessExpr*>(expr);
            markTailCalls(e->array, false, fn_name);
            markTailCalls(e->index, false, fn_name);
            break;
        }
        case HIRExpr::Kind::InlineAsm:
            break;
        case HIRExpr::Kind::FnRef:
            break;
        case HIRExpr::Kind::CallIndirect: {
            auto* e = static_cast<HIRCallIndirectExpr*>(expr);
            e->is_tail_call = in_tail_pos;
            markTailCalls(e->callee, false, fn_name);
            for (uint32_t i = 0; i < e->arg_count; ++i)
                markTailCalls(e->args[i], false, fn_name);
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
        case HIRStmt::Kind::IndexAssign: {
            auto* s = static_cast<HIRIndexAssignStmt*>(stmt);
            markTailCalls(s->array, false, fn_name);
            markTailCalls(s->index, false, fn_name);
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
        case HIRExpr::Kind::Cast:
            return hasCallTo(static_cast<const HIRCastExpr*>(expr)->operand, fn_name);
        case HIRExpr::Kind::Loop: {
            auto* e = static_cast<const HIRLoopExpr*>(expr);
            for (uint32_t i = 0; i < e->binding_count; ++i)
                if (hasCallTo(e->bindings[i].init, fn_name)) return true;
            return hasCallTo(e->body, fn_name);
        }
        case HIRExpr::Kind::Break:
            return hasCallTo(static_cast<const HIRBreakExpr*>(expr)->value, fn_name);
        case HIRExpr::Kind::Continue: {
            auto* e = static_cast<const HIRContinueExpr*>(expr);
            for (uint32_t i = 0; i < e->arg_count; ++i)
                if (hasCallTo(e->args[i], fn_name)) return true;
            return false;
        }
        case HIRExpr::Kind::ArrayLit: {
            auto* e = static_cast<const HIRArrayLitExpr*>(expr);
            for (uint32_t i = 0; i < e->element_count; ++i)
                if (hasCallTo(e->elements[i], fn_name)) return true;
            return false;
        }
        case HIRExpr::Kind::IndexAccess: {
            auto* e = static_cast<const HIRIndexAccessExpr*>(expr);
            return hasCallTo(e->array, fn_name) || hasCallTo(e->index, fn_name);
        }
        case HIRExpr::Kind::InlineAsm:
            return false;
        case HIRExpr::Kind::FnRef:
            return false;
        case HIRExpr::Kind::CallIndirect: {
            auto* e = static_cast<const HIRCallIndirectExpr*>(expr);
            if (hasCallTo(e->callee, fn_name)) return true;
            for (uint32_t i = 0; i < e->arg_count; ++i)
                if (hasCallTo(e->args[i], fn_name)) return true;
            return false;
        }
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
        case HIRStmt::Kind::IndexAssign: {
            auto* s = static_cast<const HIRIndexAssignStmt*>(stmt);
            return hasCallTo(s->array, fn_name) || hasCallTo(s->index, fn_name) ||
                   hasCallTo(s->value, fn_name);
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
        case HIRExpr::Kind::Cast:
            return hasNonTailCall(static_cast<const HIRCastExpr*>(expr)->operand, fn_name, false);
        case HIRExpr::Kind::Loop: {
            auto* e = static_cast<const HIRLoopExpr*>(expr);
            for (uint32_t i = 0; i < e->binding_count; ++i)
                if (hasNonTailCall(e->bindings[i].init, fn_name, false)) return true;
            return hasNonTailCall(e->body, fn_name, in_tail_pos);
        }
        case HIRExpr::Kind::Break:
            return hasNonTailCall(static_cast<const HIRBreakExpr*>(expr)->value, fn_name, false);
        case HIRExpr::Kind::Continue: {
            auto* e = static_cast<const HIRContinueExpr*>(expr);
            for (uint32_t i = 0; i < e->arg_count; ++i)
                if (hasNonTailCall(e->args[i], fn_name, false)) return true;
            return false;
        }
        case HIRExpr::Kind::ArrayLit: {
            auto* e = static_cast<const HIRArrayLitExpr*>(expr);
            for (uint32_t i = 0; i < e->element_count; ++i)
                if (hasNonTailCall(e->elements[i], fn_name, false)) return true;
            return false;
        }
        case HIRExpr::Kind::IndexAccess: {
            auto* e = static_cast<const HIRIndexAccessExpr*>(expr);
            return hasNonTailCall(e->array, fn_name, false) ||
                   hasNonTailCall(e->index, fn_name, false);
        }
        case HIRExpr::Kind::InlineAsm:
            return false;
        case HIRExpr::Kind::FnRef:
            return false;
        case HIRExpr::Kind::CallIndirect: {
            auto* e = static_cast<const HIRCallIndirectExpr*>(expr);
            // Indirect calls cannot be self-recursive (unknown callee), just traverse subexprs
            if (hasNonTailCall(e->callee, fn_name, false)) return true;
            for (uint32_t i = 0; i < e->arg_count; ++i)
                if (hasNonTailCall(e->args[i], fn_name, false)) return true;
            return false;
        }
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
        case HIRStmt::Kind::IndexAssign: {
            auto* s = static_cast<const HIRIndexAssignStmt*>(stmt);
            return hasNonTailCall(s->array, fn_name, false) ||
                   hasNonTailCall(s->index, fn_name, false) ||
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

// ============================================================================
// ConstOverflowPass
// ============================================================================

static void checkConstOverflow(const HIRExpr* expr, CompilationContext& ctx);

static void checkConstOverflowStmt(const HIRStmt* stmt, CompilationContext& ctx) {
    if (!stmt) return;
    switch (stmt->kind) {
    case HIRStmt::Kind::ValDecl:
        checkConstOverflow(static_cast<const HIRValDeclStmt*>(stmt)->init, ctx);
        break;
    case HIRStmt::Kind::VarDecl:
        checkConstOverflow(static_cast<const HIRVarDeclStmt*>(stmt)->init, ctx);
        break;
    case HIRStmt::Kind::ExprStmt:
        checkConstOverflow(static_cast<const HIRExprStmt*>(stmt)->expr, ctx);
        break;
    case HIRStmt::Kind::Assign:
        checkConstOverflow(static_cast<const HIRAssignStmt*>(stmt)->value, ctx);
        break;
    case HIRStmt::Kind::FieldAssign:
        checkConstOverflow(static_cast<const HIRFieldAssignStmt*>(stmt)->value, ctx);
        break;
    case HIRStmt::Kind::DerefAssign:
        checkConstOverflow(static_cast<const HIRDerefAssignStmt*>(stmt)->value, ctx);
        break;
    case HIRStmt::Kind::IndexAssign:
        checkConstOverflow(static_cast<const HIRIndexAssignStmt*>(stmt)->value, ctx);
        break;
    }
}

static void checkConstOverflow(const HIRExpr* expr, CompilationContext& ctx) {
    if (!expr) return;
    switch (expr->kind) {
    case HIRExpr::Kind::BinOp: {
        auto* bin = static_cast<const HIRBinOpExpr*>(expr);
        checkConstOverflow(bin->lhs, ctx);
        checkConstOverflow(bin->rhs, ctx);

        // Check if both operands are constant integers
        if (bin->lhs->kind == HIRExpr::Kind::IntLit &&
            bin->rhs->kind == HIRExpr::Kind::IntLit &&
            ctx.types.isInteger(bin->type) && bin->type != TypeTable::Error) {
            int64_t l = static_cast<const HIRIntLitExpr*>(bin->lhs)->value;
            int64_t r = static_cast<const HIRIntLitExpr*>(bin->rhs)->value;
            int64_t result = 0;
            bool overflow = false;

            switch (bin->op) {
            case HIRBinOp::Add: result = l + r; break;
            case HIRBinOp::Sub: result = l - r; break;
            case HIRBinOp::Mul: result = l * r; break;
            default: return; // only check arith ops
            }

            auto range = ctx.types.intRange(bin->type);
            if (range.min == 0 && range.max == 0) return;

            if (ctx.types.isSigned(bin->type)) {
                overflow = result < range.min ||
                           result > static_cast<int64_t>(range.max);
            } else {
                auto uresult = static_cast<uint64_t>(result);
                overflow = uresult > range.max;
            }

            if (overflow) {
                ctx.diag.error(expr->loc,
                    std::string("constant integer overflow: result ") +
                    std::to_string(result) + " does not fit in " +
                    ctx.types.name(bin->type));
            }
        }
        break;
    }
    case HIRExpr::Kind::Block: {
        auto* blk = static_cast<const HIRBlockExpr*>(expr);
        for (uint32_t i = 0; i < blk->stmt_count; ++i)
            checkConstOverflowStmt(blk->stmts[i], ctx);
        checkConstOverflow(blk->result, ctx);
        break;
    }
    case HIRExpr::Kind::If: {
        auto* if_ = static_cast<const HIRIfExpr*>(expr);
        checkConstOverflow(if_->condition, ctx);
        checkConstOverflow(if_->then_branch, ctx);
        checkConstOverflow(if_->else_branch, ctx);
        break;
    }
    case HIRExpr::Kind::Call: {
        auto* call = static_cast<const HIRCallExpr*>(expr);
        for (uint32_t i = 0; i < call->arg_count; ++i)
            checkConstOverflow(call->args[i], ctx);
        break;
    }
    case HIRExpr::Kind::Return:
        checkConstOverflow(static_cast<const HIRReturnExpr*>(expr)->value, ctx);
        break;
    case HIRExpr::Kind::Match: {
        auto* m = static_cast<const HIRMatchExpr*>(expr);
        checkConstOverflow(m->scrutinee, ctx);
        for (uint32_t i = 0; i < m->arm_count; ++i)
            checkConstOverflow(m->arms[i].body, ctx);
        break;
    }
    default: break;
    }
}

void ConstOverflowPass::run(HIRModule& module, CompilationContext& ctx) {
    for (uint32_t i = 0; i < module.fn_count; ++i) {
        checkConstOverflow(module.functions[i]->body, ctx);
    }
}

// ============================================================================
// LossyCastPass
// ============================================================================

static void checkLossyCast(const HIRExpr* expr, CompilationContext& ctx);

static void checkLossyCastStmt(const HIRStmt* stmt, CompilationContext& ctx) {
    if (!stmt) return;
    switch (stmt->kind) {
    case HIRStmt::Kind::ValDecl:
        checkLossyCast(static_cast<const HIRValDeclStmt*>(stmt)->init, ctx);
        break;
    case HIRStmt::Kind::VarDecl:
        checkLossyCast(static_cast<const HIRVarDeclStmt*>(stmt)->init, ctx);
        break;
    case HIRStmt::Kind::ExprStmt:
        checkLossyCast(static_cast<const HIRExprStmt*>(stmt)->expr, ctx);
        break;
    case HIRStmt::Kind::Assign:
        checkLossyCast(static_cast<const HIRAssignStmt*>(stmt)->value, ctx);
        break;
    case HIRStmt::Kind::FieldAssign:
        checkLossyCast(static_cast<const HIRFieldAssignStmt*>(stmt)->value, ctx);
        break;
    case HIRStmt::Kind::DerefAssign:
        checkLossyCast(static_cast<const HIRDerefAssignStmt*>(stmt)->value, ctx);
        break;
    case HIRStmt::Kind::IndexAssign:
        checkLossyCast(static_cast<const HIRIndexAssignStmt*>(stmt)->value, ctx);
        break;
    }
}

static void checkLossyCast(const HIRExpr* expr, CompilationContext& ctx) {
    if (!expr) return;
    switch (expr->kind) {
    case HIRExpr::Kind::Cast: {
        auto* cast = static_cast<const HIRCastExpr*>(expr);
        checkLossyCast(cast->operand, ctx);

        if (cast->is_explicit_truncate) return;

        TypeId src = cast->operand->type;
        TypeId dst = cast->target_type;
        if (!ctx.types.isInteger(src) || !ctx.types.isInteger(dst)) return;

        uint32_t src_bits = ctx.types.bitWidth(src);
        uint32_t dst_bits = ctx.types.bitWidth(dst);
        if (dst_bits < src_bits) {
            ctx.diag.warning(expr->loc,
                std::string("narrowing cast from ") + ctx.types.name(src) +
                " to " + ctx.types.name(dst) + " may lose data; use truncate<" +
                ctx.types.name(dst) + ">() for explicit narrowing");
        }
        break;
    }
    case HIRExpr::Kind::Block: {
        auto* blk = static_cast<const HIRBlockExpr*>(expr);
        for (uint32_t i = 0; i < blk->stmt_count; ++i)
            checkLossyCastStmt(blk->stmts[i], ctx);
        checkLossyCast(blk->result, ctx);
        break;
    }
    case HIRExpr::Kind::If: {
        auto* if_ = static_cast<const HIRIfExpr*>(expr);
        checkLossyCast(if_->condition, ctx);
        checkLossyCast(if_->then_branch, ctx);
        checkLossyCast(if_->else_branch, ctx);
        break;
    }
    case HIRExpr::Kind::Call: {
        auto* call = static_cast<const HIRCallExpr*>(expr);
        for (uint32_t i = 0; i < call->arg_count; ++i)
            checkLossyCast(call->args[i], ctx);
        break;
    }
    case HIRExpr::Kind::Return:
        checkLossyCast(static_cast<const HIRReturnExpr*>(expr)->value, ctx);
        break;
    default: break;
    }
}

void LossyCastPass::run(HIRModule& module, CompilationContext& ctx) {
    for (uint32_t i = 0; i < module.fn_count; ++i) {
        checkLossyCast(module.functions[i]->body, ctx);
    }
}

// ============================================================================
// BorrowEscapePass
// ============================================================================

static void collectLocals(const HIRFnDecl* fn,
                          std::unordered_set<std::string_view>& locals) {
    // Collect param names
    for (uint32_t i = 0; i < fn->param_count; ++i)
        locals.insert(fn->params[i].name);
}

static void collectLocalBindings(const HIRExpr* expr,
                                  std::unordered_set<std::string_view>& locals);

static void collectLocalBindingsStmt(const HIRStmt* stmt,
                                      std::unordered_set<std::string_view>& locals) {
    if (!stmt) return;
    if (stmt->kind == HIRStmt::Kind::ValDecl)
        locals.insert(static_cast<const HIRValDeclStmt*>(stmt)->name);
    else if (stmt->kind == HIRStmt::Kind::VarDecl)
        locals.insert(static_cast<const HIRVarDeclStmt*>(stmt)->name);
}

static void collectLocalBindings(const HIRExpr* expr,
                                  std::unordered_set<std::string_view>& locals) {
    if (!expr) return;
    if (expr->kind == HIRExpr::Kind::Block) {
        auto* blk = static_cast<const HIRBlockExpr*>(expr);
        for (uint32_t i = 0; i < blk->stmt_count; ++i)
            collectLocalBindingsStmt(blk->stmts[i], locals);
        collectLocalBindings(blk->result, locals);
    }
}

static bool isLocalAddrOf(const HIRExpr* expr,
                           const std::unordered_set<std::string_view>& locals) {
    if (!expr) return false;
    if (expr->kind == HIRExpr::Kind::AddrOf) {
        auto* ao = static_cast<const HIRAddrOfExpr*>(expr);
        if (ao->operand && ao->operand->kind == HIRExpr::Kind::Ident) {
            auto* id = static_cast<const HIRIdentExpr*>(ao->operand);
            return locals.count(id->name) > 0;
        }
    }
    return false;
}

static std::string_view getAddrOfName(const HIRExpr* expr) {
    auto* ao = static_cast<const HIRAddrOfExpr*>(expr);
    return static_cast<const HIRIdentExpr*>(ao->operand)->name;
}

static void checkBorrowEscape(const HIRExpr* expr,
                               const std::unordered_set<std::string_view>& locals,
                               CompilationContext& ctx);

static void checkBorrowEscapeStmt(const HIRStmt* stmt,
                                    const std::unordered_set<std::string_view>& locals,
                                    CompilationContext& ctx) {
    if (!stmt) return;
    switch (stmt->kind) {
    case HIRStmt::Kind::ValDecl:
        checkBorrowEscape(static_cast<const HIRValDeclStmt*>(stmt)->init, locals, ctx);
        break;
    case HIRStmt::Kind::VarDecl:
        checkBorrowEscape(static_cast<const HIRVarDeclStmt*>(stmt)->init, locals, ctx);
        break;
    case HIRStmt::Kind::ExprStmt:
        checkBorrowEscape(static_cast<const HIRExprStmt*>(stmt)->expr, locals, ctx);
        break;
    case HIRStmt::Kind::Assign:
        checkBorrowEscape(static_cast<const HIRAssignStmt*>(stmt)->value, locals, ctx);
        break;
    case HIRStmt::Kind::FieldAssign:
        checkBorrowEscape(static_cast<const HIRFieldAssignStmt*>(stmt)->value, locals, ctx);
        break;
    case HIRStmt::Kind::DerefAssign:
        checkBorrowEscape(static_cast<const HIRDerefAssignStmt*>(stmt)->value, locals, ctx);
        break;
    case HIRStmt::Kind::IndexAssign:
        checkBorrowEscape(static_cast<const HIRIndexAssignStmt*>(stmt)->value, locals, ctx);
        break;
    }
}

static void checkBorrowEscape(const HIRExpr* expr,
                               const std::unordered_set<std::string_view>& locals,
                               CompilationContext& ctx) {
    if (!expr) return;
    switch (expr->kind) {
    case HIRExpr::Kind::Return: {
        auto* ret = static_cast<const HIRReturnExpr*>(expr);
        if (isLocalAddrOf(ret->value, locals)) {
            ctx.diag.error(expr->loc,
                std::string("cannot return reference to local variable '") +
                std::string(getAddrOfName(ret->value)) + "'");
        }
        checkBorrowEscape(ret->value, locals, ctx);
        break;
    }
    case HIRExpr::Kind::Block: {
        auto* blk = static_cast<const HIRBlockExpr*>(expr);
        for (uint32_t i = 0; i < blk->stmt_count; ++i)
            checkBorrowEscapeStmt(blk->stmts[i], locals, ctx);
        checkBorrowEscape(blk->result, locals, ctx);
        break;
    }
    case HIRExpr::Kind::If: {
        auto* if_ = static_cast<const HIRIfExpr*>(expr);
        checkBorrowEscape(if_->condition, locals, ctx);
        checkBorrowEscape(if_->then_branch, locals, ctx);
        checkBorrowEscape(if_->else_branch, locals, ctx);
        break;
    }
    case HIRExpr::Kind::Call: {
        auto* call = static_cast<const HIRCallExpr*>(expr);
        for (uint32_t i = 0; i < call->arg_count; ++i)
            checkBorrowEscape(call->args[i], locals, ctx);
        break;
    }
    case HIRExpr::Kind::Match: {
        auto* m = static_cast<const HIRMatchExpr*>(expr);
        checkBorrowEscape(m->scrutinee, locals, ctx);
        for (uint32_t i = 0; i < m->arm_count; ++i)
            checkBorrowEscape(m->arms[i].body, locals, ctx);
        break;
    }
    default: break;
    }
}

void BorrowEscapePass::run(HIRModule& module, CompilationContext& ctx) {
    for (uint32_t i = 0; i < module.fn_count; ++i) {
        auto* fn = module.functions[i];
        if (!fn->body) continue;

        std::unordered_set<std::string_view> locals;
        collectLocals(fn, locals);
        collectLocalBindings(fn->body, locals);
        checkBorrowEscape(fn->body, locals, ctx);
    }
}

// ============================================================================
// MutBorrowAliasPass
// ============================================================================

static std::string_view getIdentName(const HIRExpr* expr) {
    if (expr && expr->kind == HIRExpr::Kind::Ident)
        return static_cast<const HIRIdentExpr*>(expr)->name;
    return {};
}

static void checkMutBorrowAlias(const HIRExpr* expr,
                                 const std::unordered_map<std::string_view, const HIRFnDecl*>& fn_map,
                                 CompilationContext& ctx);

static void checkMutBorrowAliasStmt(const HIRStmt* stmt,
                                     const std::unordered_map<std::string_view, const HIRFnDecl*>& fn_map,
                                     CompilationContext& ctx) {
    if (!stmt) return;
    switch (stmt->kind) {
    case HIRStmt::Kind::ValDecl:
        checkMutBorrowAlias(static_cast<const HIRValDeclStmt*>(stmt)->init, fn_map, ctx);
        break;
    case HIRStmt::Kind::VarDecl:
        checkMutBorrowAlias(static_cast<const HIRVarDeclStmt*>(stmt)->init, fn_map, ctx);
        break;
    case HIRStmt::Kind::ExprStmt:
        checkMutBorrowAlias(static_cast<const HIRExprStmt*>(stmt)->expr, fn_map, ctx);
        break;
    case HIRStmt::Kind::Assign:
        checkMutBorrowAlias(static_cast<const HIRAssignStmt*>(stmt)->value, fn_map, ctx);
        break;
    case HIRStmt::Kind::FieldAssign:
        checkMutBorrowAlias(static_cast<const HIRFieldAssignStmt*>(stmt)->value, fn_map, ctx);
        break;
    case HIRStmt::Kind::DerefAssign:
        checkMutBorrowAlias(static_cast<const HIRDerefAssignStmt*>(stmt)->value, fn_map, ctx);
        break;
    case HIRStmt::Kind::IndexAssign:
        checkMutBorrowAlias(static_cast<const HIRIndexAssignStmt*>(stmt)->value, fn_map, ctx);
        break;
    }
}

static void checkMutBorrowAlias(const HIRExpr* expr,
                                 const std::unordered_map<std::string_view, const HIRFnDecl*>& fn_map,
                                 CompilationContext& ctx) {
    if (!expr) return;
    switch (expr->kind) {
    case HIRExpr::Kind::Call: {
        auto* call = static_cast<const HIRCallExpr*>(expr);
        // Recurse into args
        for (uint32_t i = 0; i < call->arg_count; ++i)
            checkMutBorrowAlias(call->args[i], fn_map, ctx);

        // Look up callee's declaration
        auto it = fn_map.find(call->callee);
        if (it == fn_map.end()) break;
        auto* callee = it->second;

        // Collect identifiers passed to var-mode params
        std::unordered_map<std::string_view, uint32_t> var_args;
        for (uint32_t i = 0; i < call->arg_count && i < callee->param_count; ++i) {
            if (callee->params[i].passing_mode == 1) { // MutBorrow
                auto name = getIdentName(call->args[i]);
                if (!name.empty()) {
                    if (var_args.count(name)) {
                        ctx.diag.error(expr->loc,
                            std::string("mutable borrow alias: '") +
                            std::string(name) +
                            "' passed to multiple var parameters");
                        break;
                    }
                    var_args[name] = i;
                }
            }
        }
        break;
    }
    case HIRExpr::Kind::Block: {
        auto* blk = static_cast<const HIRBlockExpr*>(expr);
        for (uint32_t i = 0; i < blk->stmt_count; ++i)
            checkMutBorrowAliasStmt(blk->stmts[i], fn_map, ctx);
        checkMutBorrowAlias(blk->result, fn_map, ctx);
        break;
    }
    case HIRExpr::Kind::If: {
        auto* if_ = static_cast<const HIRIfExpr*>(expr);
        checkMutBorrowAlias(if_->condition, fn_map, ctx);
        checkMutBorrowAlias(if_->then_branch, fn_map, ctx);
        checkMutBorrowAlias(if_->else_branch, fn_map, ctx);
        break;
    }
    case HIRExpr::Kind::Return:
        checkMutBorrowAlias(static_cast<const HIRReturnExpr*>(expr)->value, fn_map, ctx);
        break;
    case HIRExpr::Kind::Match: {
        auto* m = static_cast<const HIRMatchExpr*>(expr);
        checkMutBorrowAlias(m->scrutinee, fn_map, ctx);
        for (uint32_t i = 0; i < m->arm_count; ++i)
            checkMutBorrowAlias(m->arms[i].body, fn_map, ctx);
        break;
    }
    default: break;
    }
}

void MutBorrowAliasPass::run(HIRModule& module, CompilationContext& ctx) {
    // Build callee lookup map
    std::unordered_map<std::string_view, const HIRFnDecl*> fn_map;
    for (uint32_t i = 0; i < module.fn_count; ++i)
        fn_map[module.functions[i]->name] = module.functions[i];

    for (uint32_t i = 0; i < module.fn_count; ++i) {
        if (module.functions[i]->body)
            checkMutBorrowAlias(module.functions[i]->body, fn_map, ctx);
    }
}

// ============================================================================
// UnusedBindingPass
// ============================================================================

static void countIdents(const HIRExpr* expr,
                        std::unordered_map<std::string_view, uint32_t>& counts);

static void countIdentsStmt(const HIRStmt* stmt,
                             std::unordered_map<std::string_view, uint32_t>& counts) {
    if (!stmt) return;
    switch (stmt->kind) {
    case HIRStmt::Kind::ValDecl:
        countIdents(static_cast<const HIRValDeclStmt*>(stmt)->init, counts);
        break;
    case HIRStmt::Kind::VarDecl:
        countIdents(static_cast<const HIRVarDeclStmt*>(stmt)->init, counts);
        break;
    case HIRStmt::Kind::ExprStmt:
        countIdents(static_cast<const HIRExprStmt*>(stmt)->expr, counts);
        break;
    case HIRStmt::Kind::Assign:
        counts[static_cast<const HIRAssignStmt*>(stmt)->name]++;
        countIdents(static_cast<const HIRAssignStmt*>(stmt)->value, counts);
        break;
    case HIRStmt::Kind::FieldAssign:
        countIdents(static_cast<const HIRFieldAssignStmt*>(stmt)->target, counts);
        countIdents(static_cast<const HIRFieldAssignStmt*>(stmt)->value, counts);
        break;
    case HIRStmt::Kind::DerefAssign:
        countIdents(static_cast<const HIRDerefAssignStmt*>(stmt)->target, counts);
        countIdents(static_cast<const HIRDerefAssignStmt*>(stmt)->value, counts);
        break;
    case HIRStmt::Kind::IndexAssign:
        countIdents(static_cast<const HIRIndexAssignStmt*>(stmt)->array, counts);
        countIdents(static_cast<const HIRIndexAssignStmt*>(stmt)->index, counts);
        countIdents(static_cast<const HIRIndexAssignStmt*>(stmt)->value, counts);
        break;
    }
}

static void countIdents(const HIRExpr* expr,
                        std::unordered_map<std::string_view, uint32_t>& counts) {
    if (!expr) return;
    switch (expr->kind) {
    case HIRExpr::Kind::Ident:
        counts[static_cast<const HIRIdentExpr*>(expr)->name]++;
        break;
    case HIRExpr::Kind::BinOp: {
        auto* bin = static_cast<const HIRBinOpExpr*>(expr);
        countIdents(bin->lhs, counts);
        countIdents(bin->rhs, counts);
        break;
    }
    case HIRExpr::Kind::UnaryOp:
        countIdents(static_cast<const HIRUnaryOpExpr*>(expr)->operand, counts);
        break;
    case HIRExpr::Kind::Call: {
        auto* call = static_cast<const HIRCallExpr*>(expr);
        for (uint32_t i = 0; i < call->arg_count; ++i)
            countIdents(call->args[i], counts);
        break;
    }
    case HIRExpr::Kind::CallIndirect: {
        auto* ci = static_cast<const HIRCallIndirectExpr*>(expr);
        countIdents(ci->callee, counts);
        for (uint32_t i = 0; i < ci->arg_count; ++i)
            countIdents(ci->args[i], counts);
        break;
    }
    case HIRExpr::Kind::If: {
        auto* if_ = static_cast<const HIRIfExpr*>(expr);
        countIdents(if_->condition, counts);
        countIdents(if_->then_branch, counts);
        countIdents(if_->else_branch, counts);
        break;
    }
    case HIRExpr::Kind::Match: {
        auto* m = static_cast<const HIRMatchExpr*>(expr);
        countIdents(m->scrutinee, counts);
        for (uint32_t i = 0; i < m->arm_count; ++i)
            countIdents(m->arms[i].body, counts);
        break;
    }
    case HIRExpr::Kind::Block: {
        auto* blk = static_cast<const HIRBlockExpr*>(expr);
        for (uint32_t i = 0; i < blk->stmt_count; ++i)
            countIdentsStmt(blk->stmts[i], counts);
        countIdents(blk->result, counts);
        break;
    }
    case HIRExpr::Kind::Return:
        countIdents(static_cast<const HIRReturnExpr*>(expr)->value, counts);
        break;
    case HIRExpr::Kind::FieldAccess:
        countIdents(static_cast<const HIRFieldAccessExpr*>(expr)->object, counts);
        break;
    case HIRExpr::Kind::StructLit: {
        auto* sl = static_cast<const HIRStructLitExpr*>(expr);
        for (uint32_t i = 0; i < sl->field_count; ++i)
            countIdents(sl->fields[i].value, counts);
        break;
    }
    case HIRExpr::Kind::ArrayLit: {
        auto* al = static_cast<const HIRArrayLitExpr*>(expr);
        for (uint32_t i = 0; i < al->element_count; ++i)
            countIdents(al->elements[i], counts);
        break;
    }
    case HIRExpr::Kind::IndexAccess: {
        auto* ia = static_cast<const HIRIndexAccessExpr*>(expr);
        countIdents(ia->array, counts);
        countIdents(ia->index, counts);
        break;
    }
    case HIRExpr::Kind::Cast:
        countIdents(static_cast<const HIRCastExpr*>(expr)->operand, counts);
        break;
    case HIRExpr::Kind::Loop: {
        auto* loop = static_cast<const HIRLoopExpr*>(expr);
        for (uint32_t i = 0; i < loop->binding_count; ++i)
            countIdents(loop->bindings[i].init, counts);
        countIdents(loop->body, counts);
        break;
    }
    case HIRExpr::Kind::Break:
        countIdents(static_cast<const HIRBreakExpr*>(expr)->value, counts);
        break;
    case HIRExpr::Kind::Continue: {
        auto* c = static_cast<const HIRContinueExpr*>(expr);
        for (uint32_t i = 0; i < c->arg_count; ++i)
            countIdents(c->args[i], counts);
        break;
    }
    case HIRExpr::Kind::AddrOf:
        countIdents(static_cast<const HIRAddrOfExpr*>(expr)->operand, counts);
        break;
    case HIRExpr::Kind::Deref:
        countIdents(static_cast<const HIRDerefExpr*>(expr)->operand, counts);
        break;
    default: break;
    }
}

struct BindingInfo {
    std::string_view name;
    SourceLocation loc;
};

static void collectBindings(const HIRExpr* expr, std::vector<BindingInfo>& bindings);

static void collectBindingsStmt(const HIRStmt* stmt, std::vector<BindingInfo>& bindings) {
    if (!stmt) return;
    if (stmt->kind == HIRStmt::Kind::ValDecl) {
        auto* vd = static_cast<const HIRValDeclStmt*>(stmt);
        bindings.push_back({vd->name, vd->loc});
        collectBindings(vd->init, bindings);
    } else if (stmt->kind == HIRStmt::Kind::VarDecl) {
        auto* vd = static_cast<const HIRVarDeclStmt*>(stmt);
        bindings.push_back({vd->name, vd->loc});
        collectBindings(vd->init, bindings);
    } else if (stmt->kind == HIRStmt::Kind::ExprStmt) {
        collectBindings(static_cast<const HIRExprStmt*>(stmt)->expr, bindings);
    }
}

static void collectBindings(const HIRExpr* expr, std::vector<BindingInfo>& bindings) {
    if (!expr) return;
    if (expr->kind == HIRExpr::Kind::Block) {
        auto* blk = static_cast<const HIRBlockExpr*>(expr);
        for (uint32_t i = 0; i < blk->stmt_count; ++i)
            collectBindingsStmt(blk->stmts[i], bindings);
        collectBindings(blk->result, bindings);
    } else if (expr->kind == HIRExpr::Kind::If) {
        auto* if_ = static_cast<const HIRIfExpr*>(expr);
        collectBindings(if_->then_branch, bindings);
        collectBindings(if_->else_branch, bindings);
    } else if (expr->kind == HIRExpr::Kind::Match) {
        auto* m = static_cast<const HIRMatchExpr*>(expr);
        for (uint32_t i = 0; i < m->arm_count; ++i)
            collectBindings(m->arms[i].body, bindings);
    } else if (expr->kind == HIRExpr::Kind::Loop) {
        auto* loop = static_cast<const HIRLoopExpr*>(expr);
        for (uint32_t i = 0; i < loop->binding_count; ++i)
            bindings.push_back({loop->bindings[i].name, loop->bindings[i].loc});
        collectBindings(loop->body, bindings);
    }
}

void UnusedBindingPass::run(HIRModule& module, CompilationContext& ctx) {
    for (uint32_t fi = 0; fi < module.fn_count; ++fi) {
        auto* fn = module.functions[fi];
        if (!fn->body) continue;

        // Collect all bindings
        std::vector<BindingInfo> bindings;
        collectBindings(fn->body, bindings);

        // Count identifier uses
        std::unordered_map<std::string_view, uint32_t> counts;
        countIdents(fn->body, counts);

        // Report unused
        for (auto& b : bindings) {
            if (b.name.empty()) continue;
            if (b.name[0] == '_') continue; // underscore-prefixed are exempt
            if (counts.find(b.name) == counts.end() || counts[b.name] == 0) {
                ctx.diag.warning(b.loc,
                    std::string("unused binding '") + std::string(b.name) + "'");
            }
        }
    }
}

} // namespace kern
