//
// Created by lambda on 11/1/25.
//

#include "ast_json_exporter.h"

#include "01_tokenize/token_converter.h"
#include "02_parsing/ast/expression/call_expression.h"
#include "02_parsing/ast/expression/function_expression.h"
#include "02_parsing/ast/expression/identifier_expression.h"
#include "02_parsing/ast/expression/if_expression.h"
#include "02_parsing/ast/expression/infix_expression.h"
#include "02_parsing/ast/expression/number_expression.h"
#include "02_parsing/ast/expression/prefix_expression.h"
#include "02_parsing/ast/expression/string_expression.h"
#include "02_parsing/ast/expression/type_expression.h"
#include "02_parsing/ast/module/program.h"
#include "02_parsing/ast/statement/block_statement.h"
#include "02_parsing/ast/statement/expression_statement.h"
#include "02_parsing/ast/statement/return_statement.h"
#include "02_parsing/ast/statement/variable_statement.h"

string_t ASTJsonExporter::toString() const {
    if (m_stack.empty()) {
        return "{}";
    }
    return m_stack.back().dump();
}

nlohmann::json ASTJsonExporter::toJson() const {
    if (m_stack.empty()) {
        return nlohmann::json::object();
    }
    return m_stack.back();
}

void ASTJsonExporter::visit(const Node<const BlockStatement> &node) {
    auto blockNode = nlohmann::json::object();
    blockNode["type"] = "blockStatement";
    blockNode["token"] = TokenConverter::exportJson(node->token());

    auto statements = nlohmann::json::array();
    for (const auto &statement: node->statements()) {
        statement->accept(*this);
        statements.push_back(pop());
    }
    blockNode["statements"] = statements;
    push(std::move(blockNode));
}

void ASTJsonExporter::visit(const Node<const CallExpression> &node) {
    auto callNode = nlohmann::json::object();
    callNode["type"] = "callExpression";
    callNode["token"] = TokenConverter::exportJson(node->token());

    node->callee()->accept(*this);
    callNode["callee"] = pop();

    auto arguments = nlohmann::json::array();
    for (const auto &argument: node->args()) {
        argument->accept(*this);
        arguments.push_back(pop());
    }
    callNode["arguments"] = arguments;

    push(std::move(callNode));
}

void ASTJsonExporter::visit(const Node<const InfixExpression> &node) {
    auto infixNode = nlohmann::json::object();
    infixNode["type"] = "infixExpression";
    infixNode["token"] = TokenConverter::exportJson(node->token());

    node->left()->accept(*this);
    infixNode["left"] = pop();

    infixNode["operator"] = node->token().literal();

    node->right()->accept(*this);
    infixNode["right"] = pop();

    push(std::move(infixNode));
}

void ASTJsonExporter::visit(const Node<const FunctionExpression> &node) {
    auto functionNode = nlohmann::json::object();
    functionNode["type"] = "functionExpression";
    functionNode["token"] = TokenConverter::exportJson(node->token());

    node->name()->accept(*this);
    functionNode["name"] = pop();

    auto parameters = nlohmann::json::array();
    for (const auto &parameter: node->parameters()) {
        parameter->accept(*this);
        parameters.push_back(pop());
    }
    functionNode["parameters"] = parameters;

    node->body()->accept(*this);
    functionNode["body"] = pop();

    push(std::move(functionNode));
}

void ASTJsonExporter::visit(const Node<const PrefixExpression> &node) {
    auto prefixNode = nlohmann::json::object();
    prefixNode["type"] = "prefixExpression";
    prefixNode["token"] = TokenConverter::exportJson(node->token());

    node->right()->accept(*this);
    prefixNode["right"] = pop();

    prefixNode["operator"] = node->token().literal();

    push(std::move(prefixNode));
}

void ASTJsonExporter::visit(const Node<const Program> &node) {
    auto programNode = nlohmann::json::object();
    programNode["type"] = "program";
    programNode["token"] = TokenConverter::exportJson(node->token());

    auto statements = nlohmann::json::array();
    for (const auto &statement: node->statements()) {
        statement->accept(*this);
        statements.push_back(pop());
    }
    programNode["statements"] = statements;
    push(std::move(programNode));
}

void ASTJsonExporter::visit(const Node<const IfExpression> &node) {
    auto ifNode = nlohmann::json::object();
    ifNode["type"] = "ifExpression";
    ifNode["token"] = TokenConverter::exportJson(node->token());

    if (node->condition() != nullptr) {
        node->condition()->accept(*this);
        ifNode["condition"] = pop();
    } else {
        ifNode["condition"] = "null";
    }

    node->consequence()->accept(*this);
    ifNode["consequence"] = pop();

    if (node->then() != nullptr) {
        node->then()->accept(*this);
        ifNode["then"] = pop();
    } else {
        ifNode["then"] = "null";
    }

    if (node->alternative() != nullptr) {
        node->alternative()->accept(*this);
        ifNode["alternative"] = pop();
    } else {
        ifNode["alternative"] = "null";
    }

    push(std::move(ifNode));
}

void ASTJsonExporter::visit(const Node<const StringExpression> &node) {
    auto stringNode = nlohmann::json::object();
    stringNode["type"] = "stringExpression";
    stringNode["value"] = node->token().literal();
    stringNode["token"] = TokenConverter::exportJson(node->token());

    push(std::move(stringNode));
}

void ASTJsonExporter::visit(const Node<const NumberExpression> &node) {
    auto numberNode = nlohmann::json::object();
    numberNode["type"] = "numberExpression";
    numberNode["value"] = node->token().literal();
    numberNode["token"] = TokenConverter::exportJson(node->token());

    push(std::move(numberNode));
}

void ASTJsonExporter::visit(const Node<const ExpressionStatement> &node) {
    auto expressionNode = nlohmann::json::object();
    expressionNode["type"] = "expressionStatement";
    expressionNode["token"] = TokenConverter::exportJson(node->token());

    node->expression()->accept(*this);
    expressionNode["expression"] = pop();

    push(std::move(expressionNode));
}

void ASTJsonExporter::visit(const Node<const TypeExpression> &node) {
    auto typeNode = nlohmann::json::object();
    typeNode["type"] = "typeExpression";
    typeNode["value"] = node->token().literal();
    typeNode["token"] = TokenConverter::exportJson(node->token());

    push(std::move(typeNode));
}

void ASTJsonExporter::visit(const Node<const VariableStatement> &node) {
    auto variableNode = nlohmann::json::object();
    variableNode["type"] = "variableStatement";
    variableNode["token"] = TokenConverter::exportJson(node->token());

    node->name()->accept(*this);
    variableNode["name"] = pop();

    if (node->type() == nullptr) {
        variableNode["type"] = "null";
    } else {
        node->type()->accept(*this);
        variableNode["type"] = pop();
    }

    if (node->value() == nullptr) {
        variableNode["value"] = "null";
    } else {
        node->value()->accept(*this);
        variableNode["value"] = pop();
    }

    push(std::move(variableNode));
}

void ASTJsonExporter::visit(const Node<const ReturnStatement> &node) {
    auto returnNode = nlohmann::json::object();
    returnNode["type"] = "returnStatement";
    returnNode["token"] = TokenConverter::exportJson(node->token());

    if (node->value() == nullptr) {
        returnNode["value"] = "null";
    } else {
        node->value()->accept(*this);
        returnNode["value"] = pop();
    }

    push(std::move(returnNode));
}

void ASTJsonExporter::visit(const Node<const IdentifierExpression> &node) {
    auto identifierNode = nlohmann::json::object();
    identifierNode["type"] = "identifierExpression";
    identifierNode["value"] = node->token().literal();
    identifierNode["token"] = TokenConverter::exportJson(node->token());

    push(std::move(identifierNode));
}

void ASTJsonExporter::push(nlohmann::json value) {
    m_stack.push_back(std::move(value));
}

nlohmann::json ASTJsonExporter::top() {
    return m_stack.back();
}

nlohmann::json ASTJsonExporter::pop() {
    auto value = m_stack.back();
    m_stack.pop_back();
    return value;
}
