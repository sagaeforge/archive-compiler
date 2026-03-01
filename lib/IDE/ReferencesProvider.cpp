#include "kern/ide/ReferencesProvider.h"
#include "kern/ide/IDEContext.h"
#include "kern/hir/HIR.h"

namespace kern {

static std::string_view findIdentAt(std::string_view content,
                                     uint32_t line, uint32_t column) {
    uint32_t cur_line = 1;
    size_t pos = 0;

    while (cur_line < line && pos < content.size()) {
        if (content[pos] == '\n') cur_line++;
        pos++;
    }

    size_t line_start = pos;
    size_t cursor = line_start + (column > 0 ? column - 1 : 0);
    if (cursor >= content.size()) return {};

    size_t start = cursor;
    while (start > line_start) {
        char c = content[start - 1];
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '_') {
            start--;
        } else {
            break;
        }
    }

    size_t end = cursor;
    while (end < content.size()) {
        char c = content[end];
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '_') {
            end++;
        } else {
            break;
        }
    }

    if (start == end) return {};
    return content.substr(start, end - start);
}

// Walk HIR expressions to find all uses of a name.
static void collectExprRefs(const HIRExpr* expr, std::string_view name,
                            std::vector<ReferenceLocation>& refs) {
    if (!expr) return;

    switch (expr->kind) {
        case HIRExpr::Kind::Ident: {
            auto* id = static_cast<const HIRIdentExpr*>(expr);
            if (id->name == name) {
                refs.push_back({expr->loc, false});
            }
            break;
        }
        case HIRExpr::Kind::Call: {
            auto* call = static_cast<const HIRCallExpr*>(expr);
            if (call->callee == name) {
                refs.push_back({expr->loc, false});
            }
            for (uint32_t i = 0; i < call->arg_count; ++i) {
                collectExprRefs(call->args[i], name, refs);
            }
            break;
        }
        case HIRExpr::Kind::BinOp: {
            auto* bin = static_cast<const HIRBinOpExpr*>(expr);
            collectExprRefs(bin->lhs, name, refs);
            collectExprRefs(bin->rhs, name, refs);
            break;
        }
        case HIRExpr::Kind::UnaryOp: {
            auto* un = static_cast<const HIRUnaryOpExpr*>(expr);
            collectExprRefs(un->operand, name, refs);
            break;
        }
        case HIRExpr::Kind::If: {
            auto* ife = static_cast<const HIRIfExpr*>(expr);
            collectExprRefs(ife->condition, name, refs);
            collectExprRefs(ife->then_branch, name, refs);
            collectExprRefs(ife->else_branch, name, refs);
            break;
        }
        case HIRExpr::Kind::Match: {
            auto* m = static_cast<const HIRMatchExpr*>(expr);
            collectExprRefs(m->scrutinee, name, refs);
            for (uint32_t i = 0; i < m->arm_count; ++i) {
                collectExprRefs(m->arms[i].body, name, refs);
            }
            break;
        }
        case HIRExpr::Kind::Block: {
            auto* blk = static_cast<const HIRBlockExpr*>(expr);
            for (uint32_t i = 0; i < blk->stmt_count; ++i) {
                auto* stmt = blk->stmts[i];
                switch (stmt->kind) {
                    case HIRStmt::Kind::ValDecl: {
                        auto* vd = static_cast<const HIRValDeclStmt*>(stmt);
                        collectExprRefs(vd->init, name, refs);
                        break;
                    }
                    case HIRStmt::Kind::VarDecl: {
                        auto* vd = static_cast<const HIRVarDeclStmt*>(stmt);
                        collectExprRefs(vd->init, name, refs);
                        break;
                    }
                    case HIRStmt::Kind::Assign: {
                        auto* as = static_cast<const HIRAssignStmt*>(stmt);
                        if (as->name == name) {
                            refs.push_back({stmt->loc, false});
                        }
                        collectExprRefs(as->value, name, refs);
                        break;
                    }
                    case HIRStmt::Kind::ExprStmt: {
                        auto* es = static_cast<const HIRExprStmt*>(stmt);
                        collectExprRefs(es->expr, name, refs);
                        break;
                    }
                    default:
                        break;
                }
            }
            collectExprRefs(blk->result, name, refs);
            break;
        }
        case HIRExpr::Kind::FieldAccess: {
            auto* fa = static_cast<const HIRFieldAccessExpr*>(expr);
            collectExprRefs(fa->object, name, refs);
            break;
        }
        case HIRExpr::Kind::Return: {
            auto* ret = static_cast<const HIRReturnExpr*>(expr);
            collectExprRefs(ret->value, name, refs);
            break;
        }
        default:
            break;
    }
}

std::vector<ReferenceLocation> ReferencesProvider::findReferences(
    IDEContext& ctx, std::string_view path,
    uint32_t line, uint32_t column,
    bool include_definition) {

    std::vector<ReferenceLocation> refs;

    auto content = ctx.getContent(path);
    if (content.empty()) return refs;

    auto ident = findIdentAt(content, line, column);
    if (ident.empty()) return refs;

    const HIRModule* hir = ctx.getHIR(path);
    if (!hir) return refs;

    // Find definition
    for (uint32_t i = 0; i < hir->fn_count; ++i) {
        auto* fn = hir->functions[i];
        if (fn->name == ident && include_definition) {
            refs.push_back({fn->loc, true});
        }
    }

    // Find all uses in function bodies
    for (uint32_t i = 0; i < hir->fn_count; ++i) {
        auto* fn = hir->functions[i];
        if (fn->body) {
            collectExprRefs(fn->body, ident, refs);
        }
    }

    return refs;
}

} // namespace kern
