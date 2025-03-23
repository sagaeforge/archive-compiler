#pragma once

#include "00_lib/lib/PointerHelper.hpp"
#include "01_tokenize/token/Token.h"

namespace nugdev::compiler::ast {

template <typename Return, typename... Args>
class ASTNodeVisitor;

class ASTNode : public lib::PointerHelper<ASTNode> {
public:
    virtual ~ASTNode() = default;

public:
    template <typename Return>
    Return accept(const std::shared_ptr<ASTNodeVisitor<Return()>> &visitor) {
        return visitor->visit(self());
    }
    template <typename Return, typename... Args>
    Return accept(const std::shared_ptr<ASTNodeVisitor<Return(Args...)>> &visitor, Args &&...args) {
        return visitor->visit(self(), std::forward<Args>(args)...);
    }
    template <typename... Args>
    void accept(const std::shared_ptr<ASTNodeVisitor<void(Args...)>> &visitor, Args &&...args) {
        visitor->visit(self(), std::forward<Args>(args)...);
    }

public:
    virtual const tokenize::Token &get_token() const = 0;
};
using ASTNodePtr = std::shared_ptr<ASTNode>;

class Expression : public ASTNode {};
using ExpressionPtr = std::shared_ptr<Expression>;

class Statement : public ASTNode {};
using StatementPtr = std::shared_ptr<Statement>;

class Module : public ASTNode {};
using ModulePtr = std::shared_ptr<Module>;

}  // namespace nugdev::compiler::ast
