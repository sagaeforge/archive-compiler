#include "kern/ide/CompletionProvider.h"
#include "kern/ide/IDEContext.h"
#include "kern/hir/HIR.h"

namespace kern {

// Extract the partial identifier before the cursor position.
static std::string_view extractPrefix(std::string_view content,
                                       uint32_t line, uint32_t column) {
    uint32_t cur_line = 1;
    size_t pos = 0;

    // Find the start of the target line
    while (cur_line < line && pos < content.size()) {
        if (content[pos] == '\n') cur_line++;
        pos++;
    }

    // Move to column (1-based)
    size_t line_start = pos;
    size_t cursor = line_start + (column > 0 ? column - 1 : 0);
    if (cursor > content.size()) cursor = content.size();

    // Walk backwards to find identifier start
    size_t end = cursor;
    size_t start = end;
    while (start > line_start) {
        char c = content[start - 1];
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '_') {
            start--;
        } else {
            break;
        }
    }

    return content.substr(start, end - start);
}

static bool startsWith(std::string_view s, std::string_view prefix) {
    if (prefix.empty()) return true;
    return s.size() >= prefix.size() &&
           s.substr(0, prefix.size()) == prefix;
}

void CompletionProvider::addKeywords(std::vector<CompletionItem>& items,
                                     std::string_view prefix) {
    static const char* KEYWORDS[] = {
        "fn", "val", "var", "if", "else", "match", "return",
        "struct", "enum", "union", "true", "false",
        "and", "or", "not", "intrinsic"
    };

    for (const char* kw : KEYWORDS) {
        if (startsWith(kw, prefix)) {
            items.push_back({kw, "keyword", CompletionItem::Keyword});
        }
    }
}

void CompletionProvider::addFunctions(std::vector<CompletionItem>& items,
                                      IDEContext& ctx, std::string_view path,
                                      std::string_view prefix) {
    const HIRModule* hir = ctx.getHIR(path);
    if (!hir) return;

    for (uint32_t i = 0; i < hir->fn_count; ++i) {
        auto* fn = hir->functions[i];
        if (startsWith(fn->name, prefix)) {
            // Build signature string
            std::string sig = "fn(";
            for (uint32_t j = 0; j < fn->param_count; ++j) {
                if (j > 0) sig += ", ";
                sig += ctx.context().types.name(fn->params[j].type);
            }
            sig += ") -> ";
            sig += ctx.context().types.name(fn->return_type);

            items.push_back({
                std::string(fn->name), sig, CompletionItem::Function
            });
        }
    }
}

void CompletionProvider::addTypes(std::vector<CompletionItem>& items,
                                  IDEContext& ctx, std::string_view path,
                                  std::string_view prefix) {
    const HIRModule* hir = ctx.getHIR(path);
    if (!hir) return;

    // Struct types
    for (uint32_t i = 0; i < hir->struct_count; ++i) {
        auto* s = hir->structs[i];
        if (startsWith(s->name, prefix)) {
            items.push_back({
                std::string(s->name), "struct", CompletionItem::Type
            });
        }
    }

    // Enum types
    for (uint32_t i = 0; i < hir->enum_count; ++i) {
        auto* e = hir->enums[i];
        if (startsWith(e->name, prefix)) {
            items.push_back({
                std::string(e->name), "enum", CompletionItem::Type
            });
        }
    }

    // Union types
    for (uint32_t i = 0; i < hir->union_count; ++i) {
        auto* u = hir->unions[i];
        if (startsWith(u->name, prefix)) {
            items.push_back({
                std::string(u->name), "union", CompletionItem::Type
            });
        }
    }

    // Primitive types
    static const char* PRIM_TYPES[] = {
        "i8", "i16", "i32", "i64", "u8", "u16", "u32", "u64",
        "f32", "f64", "bool", "String"
    };
    for (const char* t : PRIM_TYPES) {
        if (startsWith(t, prefix)) {
            items.push_back({t, "primitive type", CompletionItem::Type});
        }
    }
}

// Find the function containing the cursor position.
static const HIRFnDecl* findFunctionAt(const HIRModule* hir,
                                        uint32_t line) {
    const HIRFnDecl* best = nullptr;
    for (uint32_t i = 0; i < hir->fn_count; ++i) {
        auto* fn = hir->functions[i];
        if (fn->loc.line <= line) {
            if (!best || fn->loc.line > best->loc.line) {
                best = fn;
            }
        }
    }
    return best;
}

// Collect val/var declarations from a block expression.
static void collectBlockLocals(const HIRExpr* expr,
                                std::vector<std::pair<std::string_view, TypeId>>& locals) {
    if (!expr || expr->kind != HIRExpr::Kind::Block) return;
    auto* blk = static_cast<const HIRBlockExpr*>(expr);
    for (uint32_t i = 0; i < blk->stmt_count; ++i) {
        auto* stmt = blk->stmts[i];
        if (stmt->kind == HIRStmt::Kind::ValDecl) {
            auto* vd = static_cast<const HIRValDeclStmt*>(stmt);
            locals.push_back({vd->name, vd->type});
        } else if (stmt->kind == HIRStmt::Kind::VarDecl) {
            auto* vd = static_cast<const HIRVarDeclStmt*>(stmt);
            locals.push_back({vd->name, vd->type});
        }
    }
}

void CompletionProvider::addLocals(std::vector<CompletionItem>& items,
                                    IDEContext& ctx, std::string_view path,
                                    uint32_t line, uint32_t /* column */,
                                    std::string_view prefix) {
    const HIRModule* hir = ctx.getHIR(path);
    if (!hir) return;

    auto* fn = findFunctionAt(hir, line);
    if (!fn) return;

    // Add parameters
    for (uint32_t i = 0; i < fn->param_count; ++i) {
        if (startsWith(fn->params[i].name, prefix)) {
            items.push_back({
                std::string(fn->params[i].name),
                ctx.context().types.name(fn->params[i].type),
                CompletionItem::Variable
            });
        }
    }

    // Add local val/var declarations from function body
    std::vector<std::pair<std::string_view, TypeId>> locals;
    collectBlockLocals(fn->body, locals);
    for (auto& [name, type] : locals) {
        if (startsWith(name, prefix)) {
            items.push_back({
                std::string(name),
                ctx.context().types.name(type),
                CompletionItem::Variable
            });
        }
    }
}

void CompletionProvider::addFieldCompletions(
    std::vector<CompletionItem>& /* items */,
    IDEContext& /* ctx */, std::string_view /* path */,
    uint32_t /* line */, uint32_t /* column */) {
    // Field completions require resolving the type of the expression
    // before the dot. This needs cursor-position-aware type resolution
    // which will be implemented when HIR carries per-node source mapping.
}

std::vector<CompletionItem> CompletionProvider::complete(
    IDEContext& ctx, std::string_view path,
    uint32_t line, uint32_t column) {
    std::vector<CompletionItem> items;

    auto content = ctx.getContent(path);
    if (content.empty()) return items;

    auto prefix = extractPrefix(content, line, column);

    addKeywords(items, prefix);
    addFunctions(items, ctx, path, prefix);
    addTypes(items, ctx, path, prefix);
    addLocals(items, ctx, path, line, column, prefix);

    return items;
}

} // namespace kern
