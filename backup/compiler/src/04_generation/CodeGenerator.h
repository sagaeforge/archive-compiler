#pragma once

#include "00_app/lib/UnicodeString.hpp"
#include "00_app/stream/Stream.hpp"
#include "02_parsing/ast/ASTNodeVisitor.h"
#include "04_generation/context/Context.h"
#include "04_generation/instruction/Instruction.h"
#include "04_generation/instruction/Section.h"
#include "04_generation/memory/Register.hpp"

namespace nugdev::compiler::generation {

class CodeGenerator : public ast::ASTNodeVisitor {
  private:
    using Super = ast::ASTNodeVisitor;

  public:
    CodeGenerator();

  private:
    RegisterTag allocate_register();
    void free_register(const RegisterTag &tag);
    void push_result_register_tag(const RegisterTag &tag);
    RegisterTag pop_result_register_tag();
    void add_patch_table(const size_t &current_position, const PatchLabel targetTag);
    template <typename Function> void transaction(Function &&func) {
        // current section을 수정해줌.
        auto currentSection = *m_sections.current();
        func(currentSection);
        m_sections.pop();
        m_sections.push(currentSection);
    }
    MemoryTag find_variable(const lib::String &name);

  private:
    virtual void visit_program(const Super::NodePtr<ast::module::ProgramNode> &node) override;

    // Statement
    virtual void visit_block_statement(const Super::NodePtr<ast::statement::BlockStatementNode> &node) override;
    virtual void visit_break_statement(const Super::NodePtr<ast::statement::BreakStatementNode> &node) override;
    virtual void visit_continue_statement(const Super::NodePtr<ast::statement::ContinueStatementNode> &node) override;
    virtual void visit_expression_statement(const Super::NodePtr<ast::statement::ExpressionStatementNode> &node) override;
    virtual void visit_for_statement(const Super::NodePtr<ast::statement::ForStatementNode> &node) override;
    virtual void visit_let_statement(const Super::NodePtr<ast::statement::LetStatementNode> &node) override;
    virtual void visit_return_statement(const Super::NodePtr<ast::statement::ReturnStatementNode> &node) override;

    // Expression
    virtual void visit_array_literal_expression(const Super::NodePtr<ast::expression::ArrayLiteralNode> &node) override;
    virtual void visit_boolean_literal_expression(const Super::NodePtr<ast::expression::BooleanLiteralNode> &node) override;
    virtual void visit_call_expression(const Super::NodePtr<ast::expression::CallExpressionNode> &node) override;
    virtual void visit_function_expression(const Super::NodePtr<ast::expression::FunctionExpressionNode> &node) override;
    virtual void visit_identifier_expression(const Super::NodePtr<ast::expression::IdentifierLiteralNode> &node) override;
    virtual void visit_if_expression(const Super::NodePtr<ast::expression::IfExpressionNode> &node) override;
    virtual void visit_index_expression(const Super::NodePtr<ast::expression::IndexExpressionNode> &node) override;
    virtual void visit_infix_expression(const Super::NodePtr<ast::expression::InfixExpressionNode> &node) override;
    virtual void visit_number_literal_expression(const Super::NodePtr<ast::expression::NumberLiteralNode> &node) override;
    virtual void visit_postfix_expression(const Super::NodePtr<ast::expression::PostExpressionNode> &node) override;
    virtual void visit_prefix_expression(const Super::NodePtr<ast::expression::PrefixExpressionNode> &node) override;
    virtual void visit_string_literal_expression(const Super::NodePtr<ast::expression::StringLiteralNode> &node) override;
    virtual void visit_when_expression(const Super::NodePtr<ast::expression::WhenExpressionNode> &node) override;

  private:
    stream::MutableStream<CodeSection> m_sections;
    stream::MutableStream<RegisterTag> m_registers;
    stream::MutableStream<RegisterTag> m_resultRegisterTags;
};

} // namespace nugdev::compiler::generation
