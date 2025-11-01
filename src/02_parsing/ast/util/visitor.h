//
// Created by lambda on 11/1/25.
//

#pragma once

#include "02_parsing/ast/node.h"

class BlockStatement;
class CallExpression;
class InfixExpression;
class FunctionExpression;
class PrefixExpression;
class Program;
class IfExpression;
class StringExpression;
class NumberExpression;
class ExpressionStatement;
class TypeExpression;
class VariableStatement;
class ReturnStatement;
class IdentifierExpression;

class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;

public:
    virtual void visit(const Node<const BlockStatement> &node) = 0;

    virtual void visit(const Node<const CallExpression> &node) = 0;

    virtual void visit(const Node<const InfixExpression> &node) = 0;

    virtual void visit(const Node<const FunctionExpression> &node) = 0;

    virtual void visit(const Node<const PrefixExpression> &node) = 0;

    virtual void visit(const Node<const Program> &node) = 0;

    virtual void visit(const Node<const IfExpression> &node) = 0;

    virtual void visit(const Node<const StringExpression> &node) = 0;

    virtual void visit(const Node<const NumberExpression> &node) = 0;

    virtual void visit(const Node<const ExpressionStatement> &node) = 0;

    virtual void visit(const Node<const TypeExpression> &node) = 0;

    virtual void visit(const Node<const VariableStatement> &node) = 0;

    virtual void visit(const Node<const ReturnStatement> &node) = 0;

    virtual void visit(const Node<const IdentifierExpression> &node) = 0;
};
