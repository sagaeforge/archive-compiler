#include "02_parsing/ast/ASTNodeVisitor.h"

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
        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::statement::BreakStatementNode>(); },
         [this](const ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) -> std::any {
             return this->visit_break_statement(node, context);
         }},
        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::statement::ContinueStatementNode>(); },
         [this](const ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) -> std::any {
             return this->visit_continue_statement(node, context);
         }},
        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::statement::ExpressionStatementNode>(); },
         [this](const ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) -> std::any {
             return this->visit_expression_statement(node, context);
         }},
        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::statement::ForStatementNode>(); },
         [this](const ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) -> std::any {
             return this->visit_for_statement(node, context);
         }},
        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::statement::LetStatementNode>(); },
         [this](const ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) -> std::any {
             return this->visit_let_statement(node, context);
         }},
        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::statement::ReturnStatementNode>(); },
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
        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::expression::FunctionExpressionNode>(); },
         [this](const ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) -> std::any {
             return this->visit_function_expression(node, context);
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
        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::expression::PostExpressionNode>(); },
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
        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::expression::WhenExpressionNode>(); },
         [this](const ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) -> std::any {
             return this->visit_when_expression(node, context);
         }},
    };
}

ASTNodeVisitor::ASTNodeVisitor(const std::vector<std::tuple<NodePredicate, NodeVisitor>> &strategies) : m_strategies(strategies) {}

std::any ASTNodeVisitor::visit(const ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) {
    if (requires_context() && context.empty()) {
        // 컨텍스트가 필요하지만 비어있는 경우 기본 컨텍스트 생성
        std::unordered_map<icu::UnicodeString, std::any> defaultContext;

        for (const auto &strategy : m_strategies) {
            if (std::get<0>(strategy)(node)) {
                return std::get<1>(strategy)(node, defaultContext);
            }
        }

        // 대응되는 케이스가 없으므로 오류 상황.
        throw std::runtime_error("No strategy found for node");
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
