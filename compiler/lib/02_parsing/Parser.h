#pragma once

#include <map>
#include <memory>
#include <typeindex>

#include "02_parsing/ast/ASTNode.h"

namespace nugdev::compiler {

namespace ast {
class ASTNodeCreateStrategy;
}  // namespace ast

namespace parsing {

class ParsingContext {
    template <typename T>
        requires std::is_base_of_v<ast::ASTNodeCreateStrategy, T>
    static std::shared_ptr<T> get_strategy() {
        return std::dynamic_pointer_cast<T>(m_strategies[std::type_index(typeid(T))]);
    }

private:
    static std::map<std::type_index, std::shared_ptr<ast::ASTNodeCreateStrategy>> m_strategies;
};

class Parser {
public:
    Parser();
    ~Parser();

public:
    ast::ModulePtr Parse(const std::vector<tokenize::Token> &tokens);

private:
    ParsingContext m_context;
};

}  // namespace parsing
}  // namespace nugdev::compiler