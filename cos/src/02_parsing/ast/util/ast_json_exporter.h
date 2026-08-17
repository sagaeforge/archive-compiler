//
// Created by lambda on 11/1/25.
//

#pragma once

#include "visitor.h"

class ASTJsonExporter final : public ASTVisitor {
public:
    ASTJsonExporter() = default;
    ~ASTJsonExporter() override = default;

public:
    [[nodiscard]] string_t toString() const;
    [[nodiscard]] nlohmann::json toJson() const;

public:
    void visit(const Node<const BlockStatement> &node) override;
    void visit(const Node<const CallExpression> &node) override;
    void visit(const Node<const InfixExpression> &node) override;
    void visit(const Node<const FunctionExpression> &node) override;
    void visit(const Node<const PrefixExpression> &node) override;
    void visit(const Node<const Program> &node) override;
    void visit(const Node<const IfExpression> &node) override;
    void visit(const Node<const StringExpression> &node) override;
    void visit(const Node<const NumberExpression> &node) override;
    void visit(const Node<const ExpressionStatement> &node) override;
    void visit(const Node<const TypeExpression> &node) override;
    void visit(const Node<const VariableStatement> &node) override;
    void visit(const Node<const ReturnStatement> &node) override;
    void visit(const Node<const IdentifierExpression> &node) override;

private:
    void push(nlohmann::json value);
    nlohmann::json top();
    nlohmann::json pop();

private:
    std::vector<nlohmann::json> m_stack;
};
