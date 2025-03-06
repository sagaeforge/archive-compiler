#pragma once

#include <any>
#include <functional>
#include <unicode/unistr.h>
#include <unordered_map>

#include "00_app/lib/PointerHelper.hpp"
#include "00_app/lib/UnicodeStringHash.h"
#include "02_parsing/ast/AST.h"

namespace nugdev::compiler::ast {

class ASTNodeVisitor : public lib::PointerHelper<ASTNodeVisitor> {
  public:
    using NodePredicate = std::function<bool(const ASTNodePtr &)>;
    using NodeVisitor = std::function<std::any(const ASTNodePtr &, const std::unordered_map<icu::UnicodeString, std::any> &)>;

  public:
    virtual std::any visit(const ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context = {});

  protected:
    // Module
    virtual std::any visit_program(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) = 0;

    // Statement
    virtual std::any visit_block_statement(const ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) = 0;
    virtual std::any visit_break_statement(const ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) = 0;
    virtual std::any visit_continue_statement(const ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) = 0;
    virtual std::any visit_expression_statement(const ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) = 0;
    virtual std::any visit_for_statement(const ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) = 0;
    virtual std::any visit_let_statement(const ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) = 0;
    virtual std::any visit_return_statement(const ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) = 0;

    // Expression
    virtual std::any visit_array_literal_expression(const ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) = 0;
    virtual std::any visit_boolean_literal_expression(const ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) = 0;
    virtual std::any visit_call_expression(const ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) = 0;
    virtual std::any visit_function_expression(const ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) = 0;
    virtual std::any visit_identifier_expression(const ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) = 0;
    virtual std::any visit_if_expression(const ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) = 0;
    virtual std::any visit_index_expression(const ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) = 0;
    virtual std::any visit_infix_expression(const ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) = 0;
    virtual std::any visit_number_literal_expression(const ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) = 0;
    virtual std::any visit_postfix_expression(const ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) = 0;
    virtual std::any visit_prefix_expression(const ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) = 0;
    virtual std::any visit_string_literal_expression(const ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) = 0;
    virtual std::any visit_when_expression(const ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) = 0;

  protected:
    virtual bool requires_context() const = 0;

  protected:
    ASTNodeVisitor();
    ASTNodeVisitor(const std::vector<std::tuple<NodePredicate, NodeVisitor>> &strategies);

  protected:
    std::vector<std::tuple<NodePredicate, NodeVisitor>> m_strategies;
};

} // namespace nugdev::compiler::ast
