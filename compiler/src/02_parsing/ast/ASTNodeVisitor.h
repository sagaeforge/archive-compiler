#pragma once

#include <any>
#include <functional>
#include <unicode/unistr.h>
#include <unordered_map>

#include "00_app/lib/PointerHelper.hpp"
#include "00_app/lib/UnicodeStringHash.h"
#include "02_parsing/ast/AST.h"
#include "02_parsing/ast/expression/array/ArrayLiteralNode.h"
#include "02_parsing/ast/expression/boolean/BooleanLiteralNode.h"
#include "02_parsing/ast/expression/call/CallExpressionNode.h"
#include "02_parsing/ast/expression/function/FunctionExpressionNode.h"
#include "02_parsing/ast/expression/identifier/IdentifierLiteralNode.h"
#include "02_parsing/ast/expression/if/IfExpressionNode.h"
#include "02_parsing/ast/expression/index/IndexExpressionNode.h"
#include "02_parsing/ast/expression/infix/InfixExpressionNode.h"
#include "02_parsing/ast/expression/number/NumberLiteralNode.h"
#include "02_parsing/ast/expression/post/PostExpressionNode.h"
#include "02_parsing/ast/expression/prefix/PrefixExpressionNode.h"
#include "02_parsing/ast/expression/string/StringLiteralNode.h"
#include "02_parsing/ast/expression/when/WhenExpressionNode.h"
#include "02_parsing/ast/module/program/ProgramNode.h"
#include "02_parsing/ast/statement/block/BlockStatementNode.h"
#include "02_parsing/ast/statement/break/BreakStatementNode.h"
#include "02_parsing/ast/statement/continue/ContinueStatementNode.h"
#include "02_parsing/ast/statement/expression/ExpressionStatementNode.h"
#include "02_parsing/ast/statement/for/ForStatementNode.h"
#include "02_parsing/ast/statement/let/LetStatementNode.h"
#include "02_parsing/ast/statement/return/ReturnStatementNode.h"

namespace nugdev::compiler::ast {

class ASTNodeVisitor : public lib::PointerHelper<ASTNodeVisitor> {
  public:
    using NodePredicate = std::function<bool(const ASTNodePtr &)>;
    using NodeVisitor = std::function<std::any(const ASTNodePtr &, const std::unordered_map<icu::UnicodeString, std::any> &)>;
    template <typename T> using NodePtr = std::shared_ptr<T>;
    using Context = std::unordered_map<icu::UnicodeString, std::any>;

  public:
    virtual std::any visit(const ASTNodePtr &node, const Context &context = {});

  protected:
    // Module
    virtual std::any visit_program(const NodePtr<ast::module::ProgramNode> &node, const Context &context) = 0;

    // Statement
    virtual std::any visit_block_statement(const NodePtr<ast::statement::BlockStatementNode> &node, const Context &context) = 0;
    virtual std::any visit_break_statement(const NodePtr<ast::statement::BreakStatementNode> &node, const Context &context) = 0;
    virtual std::any visit_continue_statement(const NodePtr<ast::statement::ContinueStatementNode> &node, const Context &context) = 0;
    virtual std::any visit_expression_statement(const NodePtr<ast::statement::ExpressionStatementNode> &node, const Context &context) = 0;
    virtual std::any visit_for_statement(const NodePtr<ast::statement::ForStatementNode> &node, const Context &context) = 0;
    virtual std::any visit_let_statement(const NodePtr<ast::statement::LetStatementNode> &node, const Context &context) = 0;
    virtual std::any visit_return_statement(const NodePtr<ast::statement::ReturnStatementNode> &node, const Context &context) = 0;

    // Expression
    virtual std::any visit_array_literal_expression(const NodePtr<ast::expression::ArrayLiteralNode> &node, const Context &context) = 0;
    virtual std::any visit_boolean_literal_expression(const NodePtr<ast::expression::BooleanLiteralNode> &node, const Context &context) = 0;
    virtual std::any visit_call_expression(const NodePtr<ast::expression::CallExpressionNode> &node, const Context &context) = 0;
    virtual std::any visit_function_expression(const NodePtr<ast::expression::FunctionExpressionNode> &node, const Context &context) = 0;
    virtual std::any visit_identifier_expression(const NodePtr<ast::expression::IdentifierLiteralNode> &node, const Context &context) = 0;
    virtual std::any visit_if_expression(const NodePtr<ast::expression::IfExpressionNode> &node, const Context &context) = 0;
    virtual std::any visit_index_expression(const NodePtr<ast::expression::IndexExpressionNode> &node, const Context &context) = 0;
    virtual std::any visit_infix_expression(const NodePtr<ast::expression::InfixExpressionNode> &node, const Context &context) = 0;
    virtual std::any visit_number_literal_expression(const NodePtr<ast::expression::NumberLiteralNode> &node, const Context &context) = 0;
    virtual std::any visit_postfix_expression(const NodePtr<ast::expression::PostExpressionNode> &node, const Context &context) = 0;
    virtual std::any visit_prefix_expression(const NodePtr<ast::expression::PrefixExpressionNode> &node, const Context &context) = 0;
    virtual std::any visit_string_literal_expression(const NodePtr<ast::expression::StringLiteralNode> &node, const Context &context) = 0;
    virtual std::any visit_when_expression(const NodePtr<ast::expression::WhenExpressionNode> &node, const Context &context) = 0;

  protected:
    virtual bool requires_context() const = 0;

  protected:
    ASTNodeVisitor();
    ASTNodeVisitor(const std::vector<std::tuple<NodePredicate, NodeVisitor>> &strategies);

  protected:
    std::vector<std::tuple<NodePredicate, NodeVisitor>> m_strategies;
};

} // namespace nugdev::compiler::ast
