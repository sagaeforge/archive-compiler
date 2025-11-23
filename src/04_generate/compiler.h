//
// Created by lambda on 11/22/25.
//

#pragma once

#include "register_allocator.h"
#include "02_parsing/ast/util/visitor.h"

class Instruction;

class Compiler final : ASTVisitor {
public:
    explicit Compiler();

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

public:
    void emitComment(const string_t &comment);
    void emitLabel(const string_t &label);
    void emit(std::shared_ptr<Instruction> instruction);
    string_t generate();

private:
    void setResultRegister(const Register &reg);
    Register getResultRegister();
    Register takeResultRegister();
    void emitArithmeticOperate(const string_t &op, const Register &leftReg, const Register &rightReg);
    void emitComparisonOperate(const string_t &op, const Register &leftReg, const Register &rightReg);
    string_t newLabel(const string_t &prefix);

private:
    struct Result {
        std::optional<string_t> label;
        std::shared_ptr<Instruction> instruction;
        std::optional<string_t> comment;
    };

private:
    RegisterAllocator m_registerAllocator;
    std::vector<Result> m_results;
    std::optional<Register> m_currentResultRegister;
    std::unordered_map<string_t, string_t> m_stringLiterals;
    std::unordered_map<string_t, std::uint32_t> m_localVariables;
    std::uint32_t m_stackOffset = 0;
};
