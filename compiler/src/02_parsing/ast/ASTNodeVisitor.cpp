#include "02_parsing/ast/ASTNodeVisitor.h"

#include "02_parsing/ast/expression/array/ArrayLiteralNode.h"
#include "02_parsing/ast/expression/boolean/BooleanLiteralNode.h"
#include "02_parsing/ast/expression/call/CallExpressionNode.h"
#include "02_parsing/ast/expression/function/FunctionLiteralNode.h"
#include "02_parsing/ast/expression/identifier/IdentifierLiteralNode.h"
#include "02_parsing/ast/expression/if/IfExpressionNode.h"
#include "02_parsing/ast/expression/index/IndexExpressionNode.h"
#include "02_parsing/ast/expression/infix/InfixExpressionNode.h"
#include "02_parsing/ast/expression/number/NumberLiteralNode.h"
#include "02_parsing/ast/expression/post/PostNode.h"
#include "02_parsing/ast/expression/prefix/PrefixExpressionNode.h"
#include "02_parsing/ast/expression/string/StringLiteralNode.h"
#include "02_parsing/ast/expression/when/WhenNode.h"
#include "02_parsing/ast/module/program/ProgramNode.h"
#include "02_parsing/ast/statement/block/BlockStatementNode.h"
#include "02_parsing/ast/statement/break/BreakNode.h"
#include "02_parsing/ast/statement/continue/ContinueNode.h"
#include "02_parsing/ast/statement/expression/ExpressionStatementNode.h"
#include "02_parsing/ast/statement/for/ForNode.h"
#include "02_parsing/ast/statement/let/LetNode.h"
#include "02_parsing/ast/statement/return/ReturnNode.h"

namespace nugdev::compiler::ast {

ASTNodeVisitor::ASTNodeVisitor() {
    m_strategies = {
        // Module
        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::module::ProgramNode>(); },
         [this](const ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) -> std::any {
             return this->visit_program(node, context);
         }},

        // Statement
        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::statement::BlockStatementNode>(); },
         [this](const ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) -> std::any {
             return this->visit_block_statement(node, context);
         }},
        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::statement::BreakNode>(); },
         [this](const ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) -> std::any {
             return this->visit_break_statement(node, context);
         }},
        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::statement::ContinueNode>(); },
         [this](const ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) -> std::any {
             return this->visit_continue_statement(node, context);
         }},
        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::statement::ExpressionStatementNode>(); },
         [this](const ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) -> std::any {
             return this->visit_expression_statement(node, context);
         }},
        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::statement::ForNode>(); },
         [this](const ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) -> std::any {
             return this->visit_for_statement(node, context);
         }},
        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::statement::LetNode>(); },
         [this](const ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) -> std::any {
             return this->visit_let_statement(node, context);
         }},
        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::statement::ReturnNode>(); },
         [this](const ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) -> std::any {
             return this->visit_return_statement(node, context);
         }},

        // Expression
        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::expression::ArrayLiteralNode>(); },
         [this](const ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) -> std::any {
             return this->visit_array_literal_expression(node, context);
         }},
        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::expression::BooleanLiteralNode>(); },
         [this](const ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) -> std::any {
             return this->visit_boolean_literal_expression(node, context);
         }},
        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::expression::CallExpressionNode>(); },
         [this](const ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) -> std::any {
             return this->visit_call_expression(node, context);
         }},
        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::expression::FunctionLiteralNode>(); },
         [this](const ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) -> std::any {
             return this->visit_function_literal_expression(node, context);
         }},
        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::expression::IdentifierLiteralNode>(); },
         [this](const ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) -> std::any {
             return this->visit_identifier_expression(node, context);
         }},
        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::expression::IfExpressionNode>(); },
         [this](const ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) -> std::any {
             return this->visit_if_expression(node, context);
         }},
        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::expression::IndexExpressionNode>(); },
         [this](const ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) -> std::any {
             return this->visit_index_expression(node, context);
         }},
        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::expression::InfixExpressionNode>(); },
         [this](const ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) -> std::any {
             return this->visit_infix_expression(node, context);
         }},
        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::expression::NumberLiteralNode>(); },
         [this](const ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) -> std::any {
             return this->visit_number_literal_expression(node, context);
         }},
        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::expression::PostNode>(); },
         [this](const ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) -> std::any {
             return this->visit_postfix_expression(node, context);
         }},
        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::expression::PrefixExpressionNode>(); },
         [this](const ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) -> std::any {
             return this->visit_prefix_expression(node, context);
         }},
        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::expression::StringLiteralNode>(); },
         [this](const ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) -> std::any {
             return this->visit_string_literal_expression(node, context);
         }},
        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::expression::WhenNode>(); },
         [this](const ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) -> std::any {
             return this->visit_when_expression(node, context);
         }},
    };
}

ASTNodeVisitor::ASTNodeVisitor(const std::vector<std::tuple<NodePredicate, NodeVisitor>> &strategies) : m_strategies(strategies) {}

std::any ASTNodeVisitor::visit(const ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) {
    if (requires_context() && context.empty()) {
        throw std::runtime_error("Context is required for this node");
    }

    for (const auto &strategy : m_strategies) {
        if (std::get<0>(strategy)(node)) {
            return std::get<1>(strategy)(node, context);
        }
    }

    // 대응되는 케이스가 없으므로 오류 상황.
    throw std::runtime_error("No strategy found for node");
}

} // namespace nugdev::compiler::ast
