#pragma once

#include <any>
#include <functional>
#include <unicode/unistr.h>
#include <unordered_map>

#include "00_app/lib/PointerHelper.hpp"
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
    using NodeVisitor = std::function<void(const ASTNodePtr &)>;
    template <typename T> using NodePtr = std::shared_ptr<T>;

  public:
    virtual std::any visit(const ASTNodePtr &node);

  protected:
    // Module
    virtual void visit_program(const NodePtr<ast::module::ProgramNode> &node) = 0;

    // Statement
    virtual void visit_block_statement(const NodePtr<ast::statement::BlockStatementNode> &node) = 0;
    virtual void visit_break_statement(const NodePtr<ast::statement::BreakStatementNode> &node) = 0;
    virtual void visit_continue_statement(const NodePtr<ast::statement::ContinueStatementNode> &node) = 0;
    virtual void visit_expression_statement(const NodePtr<ast::statement::ExpressionStatementNode> &node) = 0;
    virtual void visit_for_statement(const NodePtr<ast::statement::ForStatementNode> &node) = 0;
    virtual void visit_let_statement(const NodePtr<ast::statement::LetStatementNode> &node) = 0;
    virtual void visit_return_statement(const NodePtr<ast::statement::ReturnStatementNode> &node) = 0;

    // Expression
    virtual void visit_array_literal_expression(const NodePtr<ast::expression::ArrayLiteralNode> &node) = 0;
    virtual void visit_boolean_literal_expression(const NodePtr<ast::expression::BooleanLiteralNode> &node) = 0;
    virtual void visit_call_expression(const NodePtr<ast::expression::CallExpressionNode> &node) = 0;
    virtual void visit_function_expression(const NodePtr<ast::expression::FunctionExpressionNode> &node) = 0;
    virtual void visit_identifier_expression(const NodePtr<ast::expression::IdentifierLiteralNode> &node) = 0;
    virtual void visit_if_expression(const NodePtr<ast::expression::IfExpressionNode> &node) = 0;
    virtual void visit_index_expression(const NodePtr<ast::expression::IndexExpressionNode> &node) = 0;
    virtual void visit_infix_expression(const NodePtr<ast::expression::InfixExpressionNode> &node) = 0;
    virtual void visit_number_literal_expression(const NodePtr<ast::expression::NumberLiteralNode> &node) = 0;
    virtual void visit_postfix_expression(const NodePtr<ast::expression::PostExpressionNode> &node) = 0;
    virtual void visit_prefix_expression(const NodePtr<ast::expression::PrefixExpressionNode> &node) = 0;
    virtual void visit_string_literal_expression(const NodePtr<ast::expression::StringLiteralNode> &node) = 0;
    virtual void visit_when_expression(const NodePtr<ast::expression::WhenExpressionNode> &node) = 0;

  protected:
    ASTNodeVisitor();
    ASTNodeVisitor(const std::vector<std::tuple<NodePredicate, NodeVisitor>> &strategies);

  protected:
    std::vector<std::tuple<NodePredicate, NodeVisitor>> m_strategies;
};

} // namespace nugdev::compiler::ast
