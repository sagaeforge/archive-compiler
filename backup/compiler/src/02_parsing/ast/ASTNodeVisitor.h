#pragma once

#include <functional>
#include <unicode/unistr.h>

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
#include "02_parsing/ast/expression/type/TypeLiteralNode.h"
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

template <typename Return, typename... Args> class ASTNodeVisitor;
template <typename Return, typename... Args> class ASTNodeVisitor<Return(Args...)> : public lib::PointerHelper<ASTNodeVisitor<Return(Args...)>> {
  public:
    using NodePredicate = std::function<bool(const ASTNodePtr &)>;
    using NodeVisitor = std::function<Return(const ASTNodePtr &, Args &&...)>;
    using NodeStrategy = std::tuple<NodePredicate, NodeVisitor>;
    template <typename T> using NodePtr = std::shared_ptr<T>;

  public:
    Return visit(const ASTNodePtr &node, Args &&...args) {
        for (const auto &[predicate, visitor] : m_strategies) {
            if (predicate(node)) {
                return visitor(node, std::forward<Args>(args)...);
            }
        }
        throw std::runtime_error("No strategy found for node");
    }

  protected:
    // Module
    virtual Return visit_program(const NodePtr<ast::module::ProgramNode> &node, Args &&...args) = 0;

    // Statement
    virtual Return visit_block_statement(const NodePtr<statement::BlockStatementNode> &node, Args &&...args) = 0;
    virtual Return visit_break_statement(const NodePtr<statement::BreakStatementNode> &node, Args &&...args) = 0;
    virtual Return visit_continue_statement(const NodePtr<statement::ContinueStatementNode> &node, Args &&...args) = 0;
    virtual Return visit_expression_statement(const NodePtr<statement::ExpressionStatementNode> &node, Args &&...args) = 0;
    virtual Return visit_for_statement(const NodePtr<statement::ForStatementNode> &node, Args &&...args) = 0;
    virtual Return visit_let_statement(const NodePtr<statement::LetStatementNode> &node, Args &&...args) = 0;
    virtual Return visit_return_statement(const NodePtr<statement::ReturnStatementNode> &node, Args &&...args) = 0;

    // Expression
    virtual Return visit_array_literal_expression(const NodePtr<expression::ArrayLiteralNode> &node, Args &&...args) = 0;
    virtual Return visit_boolean_literal_expression(const NodePtr<expression::BooleanLiteralNode> &node, Args &&...args) = 0;
    virtual Return visit_call_expression(const NodePtr<expression::CallExpressionNode> &node, Args &&...args) = 0;
    virtual Return visit_function_expression(const NodePtr<expression::FunctionExpressionNode> &node, Args &&...args) = 0;
    virtual Return visit_identifier_literal_expression(const NodePtr<expression::IdentifierLiteralNode> &node, Args &&...args) = 0;
    virtual Return visit_if_expression(const NodePtr<expression::IfExpressionNode> &node, Args &&...args) = 0;
    virtual Return visit_index_expression(const NodePtr<expression::IndexExpressionNode> &node, Args &&...args) = 0;
    virtual Return visit_infix_expression(const NodePtr<expression::InfixExpressionNode> &node, Args &&...args) = 0;
    virtual Return visit_number_literal_expression(const NodePtr<expression::NumberLiteralNode> &node, Args &&...args) = 0;
    virtual Return visit_postfix_expression(const NodePtr<expression::PostExpressionNode> &node, Args &&...args) = 0;
    virtual Return visit_prefix_expression(const NodePtr<expression::PrefixExpressionNode> &node, Args &&...args) = 0;
    virtual Return visit_string_literal_expression(const NodePtr<expression::StringLiteralNode> &node, Args &&...args) = 0;
    virtual Return visit_type_literal_expression(const NodePtr<expression::TypeLiteralNode> &node, Args &&...args) = 0;
    virtual Return visit_when_expression(const NodePtr<expression::WhenExpressionNode> &node, Args &&...args) = 0;

  protected:
    ASTNodeVisitor() {
        m_strategies = {// Module
                        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::module::ProgramNode>(); },
                         [this](const ASTNodePtr &node, Args &&...args) -> Return {
                             auto programNode = node->as<ast::module::ProgramNode>();
                             if (!programNode) {
                                 throw std::runtime_error("ProgramNode expected");
                             }
                             return this->visit_program(programNode, std::forward<Args>(args)...);
                         }},

                        // Statement
                        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::statement::BlockStatementNode>(); },
                         [this](const ASTNodePtr &node, Args &&...args) -> Return {
                             auto blockNode = node->as<ast::statement::BlockStatementNode>();
                             if (!blockNode) {
                                 throw std::runtime_error("BlockStatementNode expected");
                             }
                             return this->visit_block_statement(blockNode, std::forward<Args>(args)...);
                         }},
                        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::statement::BreakStatementNode>(); },
                         [this](const ASTNodePtr &node, Args &&...args) -> Return {
                             auto breakNode = node->as<ast::statement::BreakStatementNode>();
                             if (!breakNode) {
                                 throw std::runtime_error("BreakStatementNode expected");
                             }
                             return this->visit_break_statement(breakNode, std::forward<Args>(args)...);
                         }},
                        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::statement::ContinueStatementNode>(); },
                         [this](const ASTNodePtr &node, Args &&...args) -> Return {
                             auto continueNode = node->as<ast::statement::ContinueStatementNode>();
                             if (!continueNode) {
                                 throw std::runtime_error("ContinueStatementNode expected");
                             }
                             return this->visit_continue_statement(continueNode, std::forward<Args>(args)...);
                         }},
                        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::statement::ExpressionStatementNode>(); },
                         [this](const ASTNodePtr &node, Args &&...args) -> Return {
                             auto expressionNode = node->as<ast::statement::ExpressionStatementNode>();
                             if (!expressionNode) {
                                 throw std::runtime_error("ExpressionStatementNode expected");
                             }
                             return this->visit_expression_statement(expressionNode, std::forward<Args>(args)...);
                         }},
                        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::statement::ForStatementNode>(); },
                         [this](const ASTNodePtr &node, Args &&...args) -> Return {
                             auto forNode = node->as<ast::statement::ForStatementNode>();
                             if (!forNode) {
                                 throw std::runtime_error("ForStatementNode expected");
                             }
                             return this->visit_for_statement(forNode, std::forward<Args>(args)...);
                         }},
                        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::statement::LetStatementNode>(); },
                         [this](const ASTNodePtr &node, Args &&...args) -> Return {
                             auto letNode = node->as<ast::statement::LetStatementNode>();
                             if (!letNode) {
                                 throw std::runtime_error("LetStatementNode expected");
                             }
                             return this->visit_let_statement(letNode, std::forward<Args>(args)...);
                         }},
                        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::statement::ReturnStatementNode>(); },
                         [this](const ASTNodePtr &node, Args &&...args) -> Return {
                             auto returnNode = node->as<ast::statement::ReturnStatementNode>();
                             if (!returnNode) {
                                 throw std::runtime_error("ReturnStatementNode expected");
                             }
                             return this->visit_return_statement(returnNode, std::forward<Args>(args)...);
                         }},

                        // Expression
                        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::expression::ArrayLiteralNode>(); },
                         [this](const ASTNodePtr &node, Args &&...args) -> Return {
                             auto arrayNode = node->as<ast::expression::ArrayLiteralNode>();
                             if (!arrayNode) {
                                 throw std::runtime_error("ArrayLiteralNode expected");
                             }
                             return this->visit_array_literal_expression(arrayNode, std::forward<Args>(args)...);
                         }},
                        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::expression::BooleanLiteralNode>(); },
                         [this](const ASTNodePtr &node, Args &&...args) -> Return {
                             auto booleanNode = node->as<ast::expression::BooleanLiteralNode>();
                             if (!booleanNode) {
                                 throw std::runtime_error("BooleanLiteralNode expected");
                             }
                             return this->visit_boolean_literal_expression(booleanNode, std::forward<Args>(args)...);
                         }},
                        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::expression::CallExpressionNode>(); },
                         [this](const ASTNodePtr &node, Args &&...args) -> Return {
                             auto callNode = node->as<ast::expression::CallExpressionNode>();
                             if (!callNode) {
                                 throw std::runtime_error("CallExpressionNode expected");
                             }
                             return this->visit_call_expression(callNode, std::forward<Args>(args)...);
                         }},
                        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::expression::FunctionExpressionNode>(); },
                         [this](const ASTNodePtr &node, Args &&...args) -> Return {
                             auto functionNode = node->as<ast::expression::FunctionExpressionNode>();
                             if (!functionNode) {
                                 throw std::runtime_error("FunctionExpressionNode expected");
                             }
                             return this->visit_function_expression(functionNode, std::forward<Args>(args)...);
                         }},
                        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::expression::IdentifierLiteralNode>(); },
                         [this](const ASTNodePtr &node, Args &&...args) -> Return {
                             auto identifierNode = node->as<ast::expression::IdentifierLiteralNode>();
                             if (!identifierNode) {
                                 throw std::runtime_error("IdentifierLiteralNode expected");
                             }
                             return this->visit_identifier_literal_expression(identifierNode, std::forward<Args>(args)...);
                         }},
                        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::expression::IfExpressionNode>(); },
                         [this](const ASTNodePtr &node, Args &&...args) -> Return {
                             auto ifNode = node->as<ast::expression::IfExpressionNode>();
                             if (!ifNode) {
                                 throw std::runtime_error("IfExpressionNode expected");
                             }
                             return this->visit_if_expression(ifNode, std::forward<Args>(args)...);
                         }},
                        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::expression::IndexExpressionNode>(); },
                         [this](const ASTNodePtr &node, Args &&...args) -> Return {
                             auto indexNode = node->as<ast::expression::IndexExpressionNode>();
                             if (!indexNode) {
                                 throw std::runtime_error("IndexExpressionNode expected");
                             }
                             return this->visit_index_expression(indexNode, std::forward<Args>(args)...);
                         }},
                        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::expression::InfixExpressionNode>(); },
                         [this](const ASTNodePtr &node, Args &&...args) -> Return {
                             auto infixNode = node->as<ast::expression::InfixExpressionNode>();
                             if (!infixNode) {
                                 throw std::runtime_error("InfixExpressionNode expected");
                             }
                             return this->visit_infix_expression(infixNode, std::forward<Args>(args)...);
                         }},
                        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::expression::NumberLiteralNode>(); },
                         [this](const ASTNodePtr &node, Args &&...args) -> Return {
                             auto numberNode = node->as<ast::expression::NumberLiteralNode>();
                             if (!numberNode) {
                                 throw std::runtime_error("NumberLiteralNode expected");
                             }
                             return this->visit_number_literal_expression(numberNode, std::forward<Args>(args)...);
                         }},
                        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::expression::PostExpressionNode>(); },
                         [this](const ASTNodePtr &node, Args &&...args) -> Return {
                             auto postNode = node->as<ast::expression::PostExpressionNode>();
                             if (!postNode) {
                                 throw std::runtime_error("PostExpressionNode expected");
                             }
                             return this->visit_postfix_expression(postNode, std::forward<Args>(args)...);
                         }},
                        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::expression::PrefixExpressionNode>(); },
                         [this](const ASTNodePtr &node, Args &&...args) -> Return {
                             auto prefixNode = node->as<ast::expression::PrefixExpressionNode>();
                             if (!prefixNode) {
                                 throw std::runtime_error("PrefixExpressionNode expected");
                             }
                             return this->visit_prefix_expression(prefixNode, std::forward<Args>(args)...);
                         }},
                        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::expression::StringLiteralNode>(); },
                         [this](const ASTNodePtr &node, Args &&...args) -> Return {
                             auto stringNode = node->as<ast::expression::StringLiteralNode>();
                             if (!stringNode) {
                                 throw std::runtime_error("StringLiteralNode expected");
                             }
                             return this->visit_string_literal_expression(stringNode, std::forward<Args>(args)...);
                         }},
                        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::expression::WhenExpressionNode>(); },
                         [this](const ASTNodePtr &node, Args &&...args) -> Return {
                             auto whenNode = node->as<ast::expression::WhenExpressionNode>();
                             if (!whenNode) {
                                 throw std::runtime_error("WhenExpressionNode expected");
                             }
                             return this->visit_when_expression(whenNode, std::forward<Args>(args)...);
                         }},
                        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::expression::TypeLiteralNode>(); },
                         [this](const ASTNodePtr &node, Args &&...args) -> Return {
                             auto typeNode = node->as<ast::expression::TypeLiteralNode>();
                             if (!typeNode) {
                                 throw std::runtime_error("TypeLiteralNode expected");
                             }
                             return this->visit_type_literal_expression(typeNode, std::forward<Args>(args)...);
                         }}};
    }
    ASTNodeVisitor(const std::vector<NodeStrategy> &strategies);

  protected:
    std::vector<NodeStrategy> m_strategies;
};
template <typename... Args> class ASTNodeVisitor<void(Args...)> : public lib::PointerHelper<ASTNodeVisitor<void, Args...>> {
  public:
    using NodePredicate = std::function<bool(const ASTNodePtr &)>;
    using NodeVisitor = std::function<void(const ASTNodePtr &, Args &&...)>;
    using NodeStrategy = std::tuple<NodePredicate, NodeVisitor>;
    template <typename T> using NodePtr = std::shared_ptr<T>;

  public:
    void visit(const ASTNodePtr &node, Args &&...args) {
        for (const auto &[predicate, visitor] : m_strategies) {
            if (predicate(node)) {
                visitor(node, std::forward<Args>(args)...);
            }
        }
        throw std::runtime_error("No strategy found for node");
    }

  protected:
    // Module
    virtual void visit_program(const NodePtr<ast::module::ProgramNode> &node, Args &&...args) = 0;

    // Statement
    virtual void visit_block_statement(const NodePtr<statement::BlockStatementNode> &node, Args &&...args) = 0;
    virtual void visit_break_statement(const NodePtr<statement::BreakStatementNode> &node, Args &&...args) = 0;
    virtual void visit_continue_statement(const NodePtr<statement::ContinueStatementNode> &node, Args &&...args) = 0;
    virtual void visit_expression_statement(const NodePtr<statement::ExpressionStatementNode> &node, Args &&...args) = 0;
    virtual void visit_for_statement(const NodePtr<statement::ForStatementNode> &node, Args &&...args) = 0;
    virtual void visit_let_statement(const NodePtr<statement::LetStatementNode> &node, Args &&...args) = 0;
    virtual void visit_return_statement(const NodePtr<statement::ReturnStatementNode> &node, Args &&...args) = 0;

    // Expression
    virtual void visit_array_literal_expression(const NodePtr<expression::ArrayLiteralNode> &node, Args &&...args) = 0;
    virtual void visit_boolean_literal_expression(const NodePtr<expression::BooleanLiteralNode> &node, Args &&...args) = 0;
    virtual void visit_call_expression(const NodePtr<expression::CallExpressionNode> &node, Args &&...args) = 0;
    virtual void visit_function_expression(const NodePtr<expression::FunctionExpressionNode> &node, Args &&...args) = 0;
    virtual void visit_identifier_expression(const NodePtr<expression::IdentifierLiteralNode> &node, Args &&...args) = 0;
    virtual void visit_if_expression(const NodePtr<expression::IfExpressionNode> &node, Args &&...args) = 0;
    virtual void visit_index_expression(const NodePtr<expression::IndexExpressionNode> &node, Args &&...args) = 0;
    virtual void visit_infix_expression(const NodePtr<expression::InfixExpressionNode> &node, Args &&...args) = 0;
    virtual void visit_number_literal_expression(const NodePtr<expression::NumberLiteralNode> &node, Args &&...args) = 0;
    virtual void visit_postfix_expression(const NodePtr<expression::PostExpressionNode> &node, Args &&...args) = 0;
    virtual void visit_prefix_expression(const NodePtr<expression::PrefixExpressionNode> &node, Args &&...args) = 0;
    virtual void visit_string_literal_expression(const NodePtr<expression::StringLiteralNode> &node, Args &&...args) = 0;
    virtual void visit_when_expression(const NodePtr<expression::WhenExpressionNode> &node, Args &&...args) = 0;

  protected:
    ASTNodeVisitor() {
        m_strategies = {// Module
                        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::module::ProgramNode>(); },
                         [this](const ASTNodePtr &node, Args &&...args) -> void {
                             auto programNode = node->as<ast::module::ProgramNode>();
                             if (!programNode) {
                                 throw std::runtime_error("ProgramNode expected");
                             }
                             this->visit_program(programNode, std::forward<Args>(args)...);
                         }},

                        // Statement
                        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::statement::BlockStatementNode>(); },
                         [this](const ASTNodePtr &node, Args &&...args) -> void {
                             auto blockNode = node->as<ast::statement::BlockStatementNode>();
                             if (!blockNode) {
                                 throw std::runtime_error("BlockStatementNode expected");
                             }
                             this->visit_block_statement(blockNode, std::forward<Args>(args)...);
                         }},
                        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::statement::BreakStatementNode>(); },
                         [this](const ASTNodePtr &node, Args &&...args) -> void {
                             auto breakNode = node->as<ast::statement::BreakStatementNode>();
                             if (!breakNode) {
                                 throw std::runtime_error("BreakStatementNode expected");
                             }
                             this->visit_break_statement(breakNode, std::forward<Args>(args)...);
                         }},
                        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::statement::ContinueStatementNode>(); },
                         [this](const ASTNodePtr &node, Args &&...args) -> void {
                             auto continueNode = node->as<ast::statement::ContinueStatementNode>();
                             if (!continueNode) {
                                 throw std::runtime_error("ContinueStatementNode expected");
                             }
                             this->visit_continue_statement(continueNode, std::forward<Args>(args)...);
                         }},
                        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::statement::ExpressionStatementNode>(); },
                         [this](const ASTNodePtr &node, Args &&...args) -> void {
                             auto expressionNode = node->as<ast::statement::ExpressionStatementNode>();
                             if (!expressionNode) {
                                 throw std::runtime_error("ExpressionStatementNode expected");
                             }
                             this->visit_expression_statement(expressionNode, std::forward<Args>(args)...);
                         }},
                        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::statement::ForStatementNode>(); },
                         [this](const ASTNodePtr &node, Args &&...args) -> void {
                             auto forNode = node->as<ast::statement::ForStatementNode>();
                             if (!forNode) {
                                 throw std::runtime_error("ForStatementNode expected");
                             }
                             this->visit_for_statement(forNode, std::forward<Args>(args)...);
                         }},
                        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::statement::LetStatementNode>(); },
                         [this](const ASTNodePtr &node, Args &&...args) -> void {
                             auto letNode = node->as<ast::statement::LetStatementNode>();
                             if (!letNode) {
                                 throw std::runtime_error("LetStatementNode expected");
                             }
                             this->visit_let_statement(letNode, std::forward<Args>(args)...);
                         }},
                        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::statement::ReturnStatementNode>(); },
                         [this](const ASTNodePtr &node, Args &&...args) -> void {
                             auto returnNode = node->as<ast::statement::ReturnStatementNode>();
                             if (!returnNode) {
                                 throw std::runtime_error("ReturnStatementNode expected");
                             }
                             this->visit_return_statement(returnNode, std::forward<Args>(args)...);
                         }},

                        // Expression
                        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::expression::ArrayLiteralNode>(); },
                         [this](const ASTNodePtr &node, Args &&...args) -> void {
                             auto arrayNode = node->as<ast::expression::ArrayLiteralNode>();
                             if (!arrayNode) {
                                 throw std::runtime_error("ArrayLiteralNode expected");
                             }
                             this->visit_array_literal_expression(arrayNode, std::forward<Args>(args)...);
                         }},
                        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::expression::BooleanLiteralNode>(); },
                         [this](const ASTNodePtr &node, Args &&...args) -> void {
                             auto booleanNode = node->as<ast::expression::BooleanLiteralNode>();
                             if (!booleanNode) {
                                 throw std::runtime_error("BooleanLiteralNode expected");
                             }
                             this->visit_boolean_literal_expression(booleanNode, std::forward<Args>(args)...);
                         }},
                        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::expression::CallExpressionNode>(); },
                         [this](const ASTNodePtr &node, Args &&...args) -> void {
                             auto callNode = node->as<ast::expression::CallExpressionNode>();
                             if (!callNode) {
                                 throw std::runtime_error("CallExpressionNode expected");
                             }
                             this->visit_call_expression(callNode, std::forward<Args>(args)...);
                         }},
                        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::expression::FunctionExpressionNode>(); },
                         [this](const ASTNodePtr &node, Args &&...args) -> void {
                             auto functionNode = node->as<ast::expression::FunctionExpressionNode>();
                             if (!functionNode) {
                                 throw std::runtime_error("FunctionExpressionNode expected");
                             }
                             this->visit_function_expression(functionNode, std::forward<Args>(args)...);
                         }},
                        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::expression::IdentifierLiteralNode>(); },
                         [this](const ASTNodePtr &node, Args &&...args) -> void {
                             auto identifierNode = node->as<ast::expression::IdentifierLiteralNode>();
                             if (!identifierNode) {
                                 throw std::runtime_error("IdentifierLiteralNode expected");
                             }
                             this->visit_identifier_expression(identifierNode, std::forward<Args>(args)...);
                         }},
                        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::expression::IfExpressionNode>(); },
                         [this](const ASTNodePtr &node, Args &&...args) -> void {
                             auto ifNode = node->as<ast::expression::IfExpressionNode>();
                             if (!ifNode) {
                                 throw std::runtime_error("IfExpressionNode expected");
                             }
                             this->visit_if_expression(ifNode, std::forward<Args>(args)...);
                         }},
                        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::expression::IndexExpressionNode>(); },
                         [this](const ASTNodePtr &node, Args &&...args) -> void {
                             auto indexNode = node->as<ast::expression::IndexExpressionNode>();
                             if (!indexNode) {
                                 throw std::runtime_error("IndexExpressionNode expected");
                             }
                             this->visit_index_expression(indexNode, std::forward<Args>(args)...);
                         }},
                        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::expression::InfixExpressionNode>(); },
                         [this](const ASTNodePtr &node, Args &&...args) -> void {
                             auto infixNode = node->as<ast::expression::InfixExpressionNode>();
                             if (!infixNode) {
                                 throw std::runtime_error("InfixExpressionNode expected");
                             }
                             this->visit_infix_expression(infixNode, std::forward<Args>(args)...);
                         }},
                        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::expression::NumberLiteralNode>(); },
                         [this](const ASTNodePtr &node, Args &&...args) -> void {
                             auto numberNode = node->as<ast::expression::NumberLiteralNode>();
                             if (!numberNode) {
                                 throw std::runtime_error("NumberLiteralNode expected");
                             }
                             this->visit_number_literal_expression(numberNode, std::forward<Args>(args)...);
                         }},
                        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::expression::PostExpressionNode>(); },
                         [this](const ASTNodePtr &node, Args &&...args) -> void {
                             auto postNode = node->as<ast::expression::PostExpressionNode>();
                             if (!postNode) {
                                 throw std::runtime_error("PostExpressionNode expected");
                             }
                             this->visit_postfix_expression(postNode, std::forward<Args>(args)...);
                         }},
                        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::expression::PrefixExpressionNode>(); },
                         [this](const ASTNodePtr &node, Args &&...args) -> void {
                             auto prefixNode = node->as<ast::expression::PrefixExpressionNode>();
                             if (!prefixNode) {
                                 throw std::runtime_error("PrefixExpressionNode expected");
                             }
                             this->visit_prefix_expression(prefixNode, std::forward<Args>(args)...);
                         }},
                        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::expression::StringLiteralNode>(); },
                         [this](const ASTNodePtr &node, Args &&...args) -> void {
                             auto stringNode = node->as<ast::expression::StringLiteralNode>();
                             if (!stringNode) {
                                 throw std::runtime_error("StringLiteralNode expected");
                             }
                             this->visit_string_literal_expression(stringNode, std::forward<Args>(args)...);
                         }},
                        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::expression::WhenExpressionNode>(); },
                         [this](const ASTNodePtr &node, Args &&...args) -> void {
                             auto whenNode = node->as<ast::expression::WhenExpressionNode>();
                             if (!whenNode) {
                                 throw std::runtime_error("WhenExpressionNode expected");
                             }
                             this->visit_when_expression(whenNode, std::forward<Args>(args)...);
                         }},
                        {[this](const ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::expression::TypeLiteralNode>(); },
                         [this](const ASTNodePtr &node, Args &&...args) -> void {
                             auto typeNode = node->as<ast::expression::TypeLiteralNode>();
                             if (!typeNode) {
                                 throw std::runtime_error("TypeLiteralNode expected");
                             }
                             this->visit_type_literal_expression(typeNode, std::forward<Args>(args)...);
                         }}};
    }
    ASTNodeVisitor(const std::vector<NodeStrategy> &strategies);

  protected:
    std::vector<NodeStrategy> m_strategies;
};

} // namespace nugdev::compiler::ast
