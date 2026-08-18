#include "kern/ide/SemanticTokens.h"
#include "kern/ide/IDEContext.h"
#include "kern/hir/HIR.h"
#include "kern/lexer/Lexer.h"
#include "kern/lexer/Token.h"

namespace kern {

const char* semanticTokenTypeName(SemanticTokenType type) {
    switch (type) {
        case SemanticTokenType::Function:   return "function";
        case SemanticTokenType::Variable:   return "variable";
        case SemanticTokenType::Parameter:  return "parameter";
        case SemanticTokenType::Type:       return "type";
        case SemanticTokenType::Keyword:    return "keyword";
        case SemanticTokenType::Number:     return "number";
        case SemanticTokenType::String:     return "string";
        case SemanticTokenType::Operator:   return "operator";
        case SemanticTokenType::Comment:    return "comment";
        case SemanticTokenType::EnumMember: return "enumMember";
    }
    return "unknown";
}

// Collect function and type names from HIR for semantic classification.
struct SemanticClassification {
    std::vector<std::string_view> fn_names;
    std::vector<std::string_view> type_names;
};

static SemanticClassification classify(const HIRModule* hir) {
    SemanticClassification cls;
    if (!hir) return cls;

    for (uint32_t i = 0; i < hir->fn_count; ++i) {
        cls.fn_names.push_back(hir->functions[i]->name);
    }
    for (uint32_t i = 0; i < hir->struct_count; ++i) {
        cls.type_names.push_back(hir->structs[i]->name);
    }
    for (uint32_t i = 0; i < hir->enum_count; ++i) {
        cls.type_names.push_back(hir->enums[i]->name);
    }
    for (uint32_t i = 0; i < hir->union_count; ++i) {
        cls.type_names.push_back(hir->unions[i]->name);
    }
    return cls;
}

static bool isIn(std::string_view name, const std::vector<std::string_view>& list) {
    for (auto& n : list) {
        if (n == name) return true;
    }
    return false;
}

static bool isKeyword(TokenKind kind) {
    switch (kind) {
        case TokenKind::KwFn:
        case TokenKind::KwVal:
        case TokenKind::KwVar:
        case TokenKind::KwIf:
        case TokenKind::KwElse:
        case TokenKind::KwMatch:
        case TokenKind::KwReturn:
        case TokenKind::KwStruct:
        case TokenKind::KwEnum:
        case TokenKind::KwUnion:
        case TokenKind::KwAnd:
        case TokenKind::KwOr:
        case TokenKind::KwNot:
        case TokenKind::KwTrue:
        case TokenKind::KwFalse:
            return true;
        default:
            return false;
    }
}

std::vector<SemanticToken> SemanticTokensProvider::tokenize(
    IDEContext& ctx, std::string_view path) {

    std::vector<SemanticToken> tokens;

    auto content = ctx.getContent(path);
    if (content.empty()) return tokens;

    const HIRModule* hir = ctx.getHIR(path);
    auto cls = classify(hir);

    // Use the lexer to produce tokens, then classify semantically
    DiagnosticEngine diag;
    diag.setSource(content);
    Lexer lexer(content, "<semantic>", diag);

    while (true) {
        Token tok = lexer.nextToken();
        if (tok.kind == TokenKind::Eof) break;

        SemanticToken st;
        st.line = tok.loc.line;
        st.column = tok.loc.col;
        st.length = static_cast<uint32_t>(tok.text.size());

        if (isKeyword(tok.kind)) {
            st.type = SemanticTokenType::Keyword;
        } else if (tok.kind == TokenKind::IntLit ||
                   tok.kind == TokenKind::FloatLit) {
            st.type = SemanticTokenType::Number;
        } else if (tok.kind == TokenKind::StringLit) {
            st.type = SemanticTokenType::String;
        } else if (tok.kind == TokenKind::Ident) {
            // Classify identifiers
            if (isIn(tok.text, cls.fn_names)) {
                st.type = SemanticTokenType::Function;
            } else if (isIn(tok.text, cls.type_names)) {
                st.type = SemanticTokenType::Type;
            } else {
                st.type = SemanticTokenType::Variable;
            }
        } else if (tok.kind == TokenKind::Plus || tok.kind == TokenKind::Minus ||
                   tok.kind == TokenKind::Star || tok.kind == TokenKind::Slash ||
                   tok.kind == TokenKind::EqEq || tok.kind == TokenKind::NotEq ||
                   tok.kind == TokenKind::Lt || tok.kind == TokenKind::LtEq ||
                   tok.kind == TokenKind::Gt || tok.kind == TokenKind::GtEq ||
                   tok.kind == TokenKind::Pipe || tok.kind == TokenKind::Ampersand) {
            st.type = SemanticTokenType::Operator;
        } else {
            continue;  // Skip tokens we don't classify
        }

        tokens.push_back(st);
    }

    return tokens;
}

} // namespace kern
