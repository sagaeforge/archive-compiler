#pragma once

#include "00_app/stream/Stream.hpp"
#include "02_parsing/ast/ASTNodeVisitor.h"
#include "04_generation/context/Context.h"
#include "04_generation/instruction/Instruction.h"
#include "04_generation/instruction/Section.h"
#include "04_generation/register/Register.hpp"
#include <unordered_map>

namespace nugdev::compiler::generation {

class CodeGenerator : public ast::ASTNodeVisitor {
  private:
    using Super = ast::ASTNodeVisitor;

  public:
    CodeGenerator();

  private:
    virtual bool requires_context() const override;
    RegisterTag allocate_register();
    void free_register(const RegisterTag &tag);
    void push_instruction(const std::shared_ptr<Instruction> &instruction);
    void allocate_data_section_field(const DataSectionField &field);

  private:
    virtual std::any visit_program(const Super::NodePtr<ast::module::ProgramNode> &node, const Super::Context &context) override;

    // Statement
    virtual std::any visit_block_statement(const Super::NodePtr<ast::statement::BlockStatementNode> &node, const Super::Context &context) override;
    virtual std::any visit_break_statement(const Super::NodePtr<ast::statement::BreakStatementNode> &node, const Super::Context &context) override;
    virtual std::any visit_continue_statement(const Super::NodePtr<ast::statement::ContinueStatementNode> &node, const Super::Context &context) override;
    virtual std::any visit_expression_statement(const Super::NodePtr<ast::statement::ExpressionStatementNode> &node, const Super::Context &context) override;
    virtual std::any visit_for_statement(const Super::NodePtr<ast::statement::ForStatementNode> &node, const Super::Context &context) override;
    virtual std::any visit_let_statement(const Super::NodePtr<ast::statement::LetStatementNode> &node, const Super::Context &context) override;
    virtual std::any visit_return_statement(const Super::NodePtr<ast::statement::ReturnStatementNode> &node, const Super::Context &context) override;

    // Expression
    virtual std::any visit_array_literal_expression(const Super::NodePtr<ast::expression::ArrayLiteralNode> &node, const Super::Context &context) override;
    virtual std::any visit_boolean_literal_expression(const Super::NodePtr<ast::expression::BooleanLiteralNode> &node, const Super::Context &context) override;
    virtual std::any visit_call_expression(const Super::NodePtr<ast::expression::CallExpressionNode> &node, const Super::Context &context) override;
    virtual std::any visit_function_expression(const Super::NodePtr<ast::expression::FunctionExpressionNode> &node, const Super::Context &context) override;
    virtual std::any visit_identifier_expression(const Super::NodePtr<ast::expression::IdentifierLiteralNode> &node, const Super::Context &context) override;
    virtual std::any visit_if_expression(const Super::NodePtr<ast::expression::IfExpressionNode> &node, const Super::Context &context) override;
    virtual std::any visit_index_expression(const Super::NodePtr<ast::expression::IndexExpressionNode> &node, const Super::Context &context) override;
    virtual std::any visit_infix_expression(const Super::NodePtr<ast::expression::InfixExpressionNode> &node, const Super::Context &context) override;
    virtual std::any visit_number_literal_expression(const Super::NodePtr<ast::expression::NumberLiteralNode> &node, const Super::Context &context) override;
    virtual std::any visit_postfix_expression(const Super::NodePtr<ast::expression::PostExpressionNode> &node, const Super::Context &context) override;
    virtual std::any visit_prefix_expression(const Super::NodePtr<ast::expression::PrefixExpressionNode> &node, const Super::Context &context) override;
    virtual std::any visit_string_literal_expression(const Super::NodePtr<ast::expression::StringLiteralNode> &node, const Super::Context &context) override;
    virtual std::any visit_when_expression(const Super::NodePtr<ast::expression::WhenExpressionNode> &node, const Super::Context &context) override;

  private:
    stream::MutableStream<CodeSection> m_sections;
    stream::MutableStream<RegisterTag> m_registers;
};

} // namespace nugdev::compiler::generation
