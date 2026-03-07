#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace kern {

class IDEContext;

struct CompletionItem {
    std::string label;
    std::string detail;      // type info or signature
    enum Kind : uint8_t {
        Function, Variable, Type, Field, Keyword, EnumVariant
    };
    Kind kind;
};

// Provides code completion candidates at a given cursor position.

class CompletionProvider {
public:
    std::vector<CompletionItem> complete(
        IDEContext& ctx, std::string_view path,
        uint32_t line, uint32_t column);

private:
    void addKeywords(std::vector<CompletionItem>& items,
                     std::string_view prefix);
    void addFunctions(std::vector<CompletionItem>& items,
                      IDEContext& ctx, std::string_view path,
                      std::string_view prefix);
    void addTypes(std::vector<CompletionItem>& items,
                  IDEContext& ctx, std::string_view path,
                  std::string_view prefix);
    void addLocals(std::vector<CompletionItem>& items,
                   IDEContext& ctx, std::string_view path,
                   uint32_t line, uint32_t column,
                   std::string_view prefix);
    void addFieldCompletions(std::vector<CompletionItem>& items,
                             IDEContext& ctx, std::string_view path,
                             uint32_t line, uint32_t column);
};

} // namespace kern
