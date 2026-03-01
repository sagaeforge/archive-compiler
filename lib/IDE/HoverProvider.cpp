#include "kern/ide/HoverProvider.h"
#include "kern/ide/IDEContext.h"
#include "kern/hir/HIR.h"
#include "kern/ir/Metadata.h"

namespace kern {

// Find the identifier at (line, column) in source text.
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

    // Walk to start of identifier
    size_t start = cursor;
    while (start > line_start) {
        char c = content[start - 1];
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '_') {
            start--;
        } else {
            break;
        }
    }

    // Walk to end of identifier
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

std::optional<HoverResult> HoverProvider::hover(
    IDEContext& ctx, std::string_view path,
    uint32_t line, uint32_t column) {

    auto content = ctx.getContent(path);
    if (content.empty()) return std::nullopt;

    auto ident = findIdentAt(content, line, column);
    if (ident.empty()) return std::nullopt;

    const HIRModule* hir = ctx.getHIR(path);
    if (!hir) return std::nullopt;

    // Search functions
    for (uint32_t i = 0; i < hir->fn_count; ++i) {
        auto* fn = hir->functions[i];
        if (fn->name == ident) {
            HoverResult result;

            // Build type info
            std::string sig = "fn " + std::string(fn->name) + "(";
            for (uint32_t j = 0; j < fn->param_count; ++j) {
                if (j > 0) sig += ", ";
                sig += std::string(fn->params[j].name) + ": ";
                sig += ctx.context().types.name(fn->params[j].type);
            }
            sig += ") -> ";
            sig += ctx.context().types.name(fn->return_type);
            result.type_info = sig;

            result.purity = purityName(static_cast<Purity>(fn->purity));
            result.definition_loc = fn->loc;
            return result;
        }
    }

    // Search struct types
    for (uint32_t i = 0; i < hir->struct_count; ++i) {
        auto* s = hir->structs[i];
        if (s->name == ident) {
            HoverResult result;
            result.type_info = "struct " + std::string(s->name);
            result.definition_loc = s->loc;
            return result;
        }
    }

    // Search enum types
    for (uint32_t i = 0; i < hir->enum_count; ++i) {
        auto* e = hir->enums[i];
        if (e->name == ident) {
            HoverResult result;
            result.type_info = "enum " + std::string(e->name);
            result.definition_loc = e->loc;
            return result;
        }
    }

    // Search union types
    for (uint32_t i = 0; i < hir->union_count; ++i) {
        auto* u = hir->unions[i];
        if (u->name == ident) {
            HoverResult result;
            result.type_info = "union " + std::string(u->name);
            result.definition_loc = u->loc;
            return result;
        }
    }

    return std::nullopt;
}

} // namespace kern
