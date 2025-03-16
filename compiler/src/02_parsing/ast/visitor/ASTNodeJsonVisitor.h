#pragma once

#include "00_app/json/Json.hpp"
#include "00_app/lib/UnicodeString.hpp"
#include "02_parsing/ast/ASTNodeVisitor.h"

namespace nugdev::compiler::ast {

class ASTNodeJsonVisitor : public ASTNodeVisitor<json::JsonValue()> {
  public:
    ASTNodeJsonVisitor();

  public:
    lib::String to_str(const json::JsonValue &value);

  protected:
    // Module
    json::JsonValue visit_program(const NodePtr<ast::module::ProgramNode> &node) override;

    // Statement
    json::JsonValue visit_block_statement(const NodePtr<statement::BlockStatementNode> &node) override;
    json::JsonValue visit_break_statement(const NodePtr<statement::BreakStatementNode> &node) override;
    json::JsonValue visit_continue_statement(const NodePtr<statement::ContinueStatementNode> &node) override;
    json::JsonValue visit_expression_statement(const NodePtr<statement::ExpressionStatementNode> &node) override;
    json::JsonValue visit_for_statement(const NodePtr<statement::ForStatementNode> &node) override;
    json::JsonValue visit_let_statement(const NodePtr<statement::LetStatementNode> &node) override;
    json::JsonValue visit_return_statement(const NodePtr<statement::ReturnStatementNode> &node) override;

    // Expression
    json::JsonValue visit_array_literal_expression(const NodePtr<expression::ArrayLiteralNode> &node) override;
    json::JsonValue visit_boolean_literal_expression(const NodePtr<expression::BooleanLiteralNode> &node) override;
    json::JsonValue visit_call_expression(const NodePtr<expression::CallExpressionNode> &node) override;
    json::JsonValue visit_function_expression(const NodePtr<expression::FunctionExpressionNode> &node) override;
    json::JsonValue visit_identifier_literal_expression(const NodePtr<expression::IdentifierLiteralNode> &node) override;
    json::JsonValue visit_if_expression(const NodePtr<expression::IfExpressionNode> &node) override;
    json::JsonValue visit_index_expression(const NodePtr<expression::IndexExpressionNode> &node) override;
    json::JsonValue visit_infix_expression(const NodePtr<expression::InfixExpressionNode> &node) override;
    json::JsonValue visit_number_literal_expression(const NodePtr<expression::NumberLiteralNode> &node) override;
    json::JsonValue visit_postfix_expression(const NodePtr<expression::PostExpressionNode> &node) override;
    json::JsonValue visit_prefix_expression(const NodePtr<expression::PrefixExpressionNode> &node) override;
    json::JsonValue visit_string_literal_expression(const NodePtr<expression::StringLiteralNode> &node) override;
    json::JsonValue visit_type_literal_expression(const NodePtr<expression::TypeLiteralNode> &node) override;
    json::JsonValue visit_when_expression(const NodePtr<expression::WhenExpressionNode> &node) override;

  private:
    json::JsonAllocator &get_allocator();

  private:
    json::JsonDocument m_document;
};

} // namespace nugdev::compiler::ast