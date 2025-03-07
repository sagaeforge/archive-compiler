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
             auto programNode = node->as<ast::module::ProgramNode>();
             if (!programNode) {
                 throw std::runtime_error("ProgramNode expected");
             }
             return this->visit_program(programNode, context);
         }},

        // Statement
        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::statement::BlockStatementNode>(); },
         [this](const ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) -> std::any {
             auto blockNode = node->as<ast::statement::BlockStatementNode>();
             if (!blockNode) {
                 throw std::runtime_error("BlockStatementNode expected");
             }
             return this->visit_block_statement(blockNode, context);
         }},
        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::statement::BreakStatementNode>(); },
         [this](const ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) -> std::any {
             auto breakNode = node->as<ast::statement::BreakStatementNode>();
             if (!breakNode) {
                 throw std::runtime_error("BreakStatementNode expected");
             }
             return this->visit_break_statement(breakNode, context);
         }},
        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::statement::ContinueStatementNode>(); },
         [this](const ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) -> std::any {
             auto continueNode = node->as<ast::statement::ContinueStatementNode>();
             if (!continueNode) {
                 throw std::runtime_error("ContinueStatementNode expected");
             }
             return this->visit_continue_statement(continueNode, context);
         }},
        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::statement::ExpressionStatementNode>(); },
         [this](const ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) -> std::any {
             auto expressionNode = node->as<ast::statement::ExpressionStatementNode>();
             if (!expressionNode) {
                 throw std::runtime_error("ExpressionStatementNode expected");
             }
             return this->visit_expression_statement(expressionNode, context);
         }},
        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::statement::ForStatementNode>(); },
         [this](const ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) -> std::any {
             auto forNode = node->as<ast::statement::ForStatementNode>();
             if (!forNode) {
                 throw std::runtime_error("ForStatementNode expected");
             }
             return this->visit_for_statement(forNode, context);
         }},
        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::statement::LetStatementNode>(); },
         [this](const ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) -> std::any {
             auto letNode = node->as<ast::statement::LetStatementNode>();
             if (!letNode) {
                 throw std::runtime_error("LetStatementNode expected");
             }
             return this->visit_let_statement(letNode, context);
         }},
        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::statement::ReturnStatementNode>(); },
         [this](const ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) -> std::any {
             auto returnNode = node->as<ast::statement::ReturnStatementNode>();
             if (!returnNode) {
                 throw std::runtime_error("ReturnStatementNode expected");
             }
             return this->visit_return_statement(returnNode, context);
         }},

        // Expression
        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::expression::ArrayLiteralNode>(); },
         [this](const ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) -> std::any {
             auto arrayNode = node->as<ast::expression::ArrayLiteralNode>();
             if (!arrayNode) {
                 throw std::runtime_error("ArrayLiteralNode expected");
             }
             return this->visit_array_literal_expression(arrayNode, context);
         }},
        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::expression::BooleanLiteralNode>(); },
         [this](const ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) -> std::any {
             auto booleanNode = node->as<ast::expression::BooleanLiteralNode>();
             if (!booleanNode) {
                 throw std::runtime_error("BooleanLiteralNode expected");
             }
             return this->visit_boolean_literal_expression(booleanNode, context);
         }},
        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::expression::CallExpressionNode>(); },
         [this](const ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) -> std::any {
             auto callNode = node->as<ast::expression::CallExpressionNode>();
             if (!callNode) {
                 throw std::runtime_error("CallExpressionNode expected");
             }
             return this->visit_call_expression(callNode, context);
         }},
        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::expression::FunctionExpressionNode>(); },
         [this](const ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) -> std::any {
             auto functionNode = node->as<ast::expression::FunctionExpressionNode>();
             if (!functionNode) {
                 throw std::runtime_error("FunctionExpressionNode expected");
             }
             return this->visit_function_expression(functionNode, context);
         }},
        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::expression::IdentifierLiteralNode>(); },
         [this](const ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) -> std::any {
             auto identifierNode = node->as<ast::expression::IdentifierLiteralNode>();
             if (!identifierNode) {
                 throw std::runtime_error("IdentifierLiteralNode expected");
             }
             return this->visit_identifier_expression(identifierNode, context);
         }},
        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::expression::IfExpressionNode>(); },
         [this](const ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) -> std::any {
             auto ifNode = node->as<ast::expression::IfExpressionNode>();
             if (!ifNode) {
                 throw std::runtime_error("IfExpressionNode expected");
             }
             return this->visit_if_expression(ifNode, context);
         }},
        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::expression::IndexExpressionNode>(); },
         [this](const ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) -> std::any {
             auto indexNode = node->as<ast::expression::IndexExpressionNode>();
             if (!indexNode) {
                 throw std::runtime_error("IndexExpressionNode expected");
             }
             return this->visit_index_expression(indexNode, context);
         }},
        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::expression::InfixExpressionNode>(); },
         [this](const ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) -> std::any {
             auto infixNode = node->as<ast::expression::InfixExpressionNode>();
             if (!infixNode) {
                 throw std::runtime_error("InfixExpressionNode expected");
             }
             return this->visit_infix_expression(infixNode, context);
         }},
        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::expression::NumberLiteralNode>(); },
         [this](const ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) -> std::any {
             auto numberNode = node->as<ast::expression::NumberLiteralNode>();
             if (!numberNode) {
                 throw std::runtime_error("NumberLiteralNode expected");
             }
             return this->visit_number_literal_expression(numberNode, context);
         }},
        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::expression::PostExpressionNode>(); },
         [this](const ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) -> std::any {
             auto postNode = node->as<ast::expression::PostExpressionNode>();
             if (!postNode) {
                 throw std::runtime_error("PostExpressionNode expected");
             }
             return this->visit_postfix_expression(postNode, context);
         }},
        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::expression::PrefixExpressionNode>(); },
         [this](const ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) -> std::any {
             auto prefixNode = node->as<ast::expression::PrefixExpressionNode>();
             if (!prefixNode) {
                 throw std::runtime_error("PrefixExpressionNode expected");
             }
             return this->visit_prefix_expression(prefixNode, context);
         }},
        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::expression::StringLiteralNode>(); },
         [this](const ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) -> std::any {
             auto stringNode = node->as<ast::expression::StringLiteralNode>();
             if (!stringNode) {
                 throw std::runtime_error("StringLiteralNode expected");
             }
             return this->visit_string_literal_expression(stringNode, context);
         }},
        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::expression::WhenExpressionNode>(); },
         [this](const ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) -> std::any {
             auto whenNode = node->as<ast::expression::WhenExpressionNode>();
             if (!whenNode) {
                 throw std::runtime_error("WhenExpressionNode expected");
             }
             return this->visit_when_expression(whenNode, context);
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
