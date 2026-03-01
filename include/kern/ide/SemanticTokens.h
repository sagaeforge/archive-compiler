#pragma once
#include <cstdint>
#include <string_view>
#include <vector>

namespace kern {

class IDEContext;

enum class SemanticTokenType : uint8_t {
    Function, Variable, Parameter, Type, Keyword,
    Number, String, Operator, Comment, EnumMember
};

struct SemanticToken {
    uint32_t line;
    uint32_t column;
    uint32_t length;
    SemanticTokenType type;
};

const char* semanticTokenTypeName(SemanticTokenType type);

// Produces semantic tokens for syntax highlighting.
// Richer than lexer tokens — distinguishes functions from variables, etc.

class SemanticTokensProvider {
public:
    std::vector<SemanticToken> tokenize(
        IDEContext& ctx, std::string_view path);
};

} // namespace kern
