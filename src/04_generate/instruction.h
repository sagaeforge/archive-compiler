//
// Created by lambda on 11/22/25.
//

#pragma once

#include "operand.h"
#include "00_core/printable.hpp"

class Instruction : public printable {
};

struct Move final : Instruction {
    string_t m_wordSize;
    std::shared_ptr<Operand> m_left;
    std::shared_ptr<Operand> m_right;

    explicit Move(const std::shared_ptr<Operand> &left, const std::shared_ptr<Operand> &right);

    explicit Move(const string_t &wordSize, const std::shared_ptr<Operand> &left,
                  const std::shared_ptr<Operand> &right);

    void print(std::ostream &os) const override;
};

struct Push final : Instruction {
    std::shared_ptr<Operand> m_operand;

    explicit Push(const std::shared_ptr<Operand> &operand);

    void print(std::ostream &os) const override;
};

struct Add final : Instruction {
    std::shared_ptr<Operand> m_left;
    std::shared_ptr<Operand> m_right;

    explicit Add(const std::shared_ptr<Operand> &left, const std::shared_ptr<Operand> &right);

    void print(std::ostream &os) const override;
};

struct Sub final : Instruction {
    std::shared_ptr<Operand> m_left;
    std::shared_ptr<Operand> m_right;

    explicit Sub(const std::shared_ptr<Operand> &left, const std::shared_ptr<Operand> &right);

    void print(std::ostream &os) const override;
};

struct Cmp final : Instruction {
    std::shared_ptr<Operand> m_left;
    std::shared_ptr<Operand> m_right;

    explicit Cmp(const std::shared_ptr<Operand> &left, const std::shared_ptr<Operand> &right);

    void print(std::ostream &os) const override;
};

struct Movzx final : Instruction {
    std::shared_ptr<Operand> m_left;
    std::shared_ptr<Operand> m_right;

    explicit Movzx(const std::shared_ptr<Operand> &left, const std::shared_ptr<Operand> &right);

    void print(std::ostream &os) const override;
};

struct Je final : Instruction {
    string_t m_label;

    explicit Je(const string_t &label);

    void print(std::ostream &os) const override;
};

struct Pop final : Instruction {
    std::shared_ptr<Operand> m_operand;

    explicit Pop(const std::shared_ptr<Operand> &operand);

    void print(std::ostream &os) const override;
};

struct Ret final : Instruction {
    void print(std::ostream &os) const override;
};

struct Call final : Instruction {
    string_t m_label;

    explicit Call(const string_t &label);

    void print(std::ostream &os) const override;
};

struct Syscall final : Instruction {
    void print(std::ostream &os) const override;
};

struct Lea final : Instruction {
    std::shared_ptr<Operand> m_left;
    std::shared_ptr<Operand> m_right;

    explicit Lea(const std::shared_ptr<Operand> &left, const std::shared_ptr<Operand> &right);

    void print(std::ostream &os) const override;
};

struct Mul final : Instruction {
    std::shared_ptr<Operand> m_left;
    std::shared_ptr<Operand> m_right;

    explicit Mul(const std::shared_ptr<Operand> &left, const std::shared_ptr<Operand> &right);

    void print(std::ostream &os) const override;
};

struct IDiv final : Instruction {
    std::shared_ptr<Operand> m_operand;

    explicit IDiv(const std::shared_ptr<Operand> &operand);

    void print(std::ostream &os) const override;
};

struct Cqo final : Instruction {
    void print(std::ostream &os) const override;
};

struct Sete final : Instruction {
    std::shared_ptr<Operand> m_operand;

    explicit Sete(const std::shared_ptr<Operand> &operand);

    void print(std::ostream &os) const override;
};

struct Setne final : Instruction {
    std::shared_ptr<Operand> m_operand;

    explicit Setne(const std::shared_ptr<Operand> &operand);

    void print(std::ostream &os) const override;
};

struct Setl final : Instruction {
    std::shared_ptr<Operand> m_operand;

    explicit Setl(const std::shared_ptr<Operand> &operand);

    void print(std::ostream &os) const override;
};

struct Setle final : Instruction {
    std::shared_ptr<Operand> m_operand;

    explicit Setle(const std::shared_ptr<Operand> &operand);

    void print(std::ostream &os) const override;
};

struct Setg final : Instruction {
    std::shared_ptr<Operand> m_operand;

    explicit Setg(const std::shared_ptr<Operand> &operand);

    void print(std::ostream &os) const override;
};

struct Setge final : Instruction {
    std::shared_ptr<Operand> m_operand;

    explicit Setge(const std::shared_ptr<Operand> &operand);

    void print(std::ostream &os) const override;
};

struct Neg final : Instruction {
    std::shared_ptr<Operand> m_operand;

    explicit Neg(const std::shared_ptr<Operand> &operand);

    void print(std::ostream &os) const override;
};

struct Test final : Instruction {
    std::shared_ptr<Operand> m_left;
    std::shared_ptr<Operand> m_right;

    explicit Test(const std::shared_ptr<Operand> &left, const std::shared_ptr<Operand> &right);

    void print(std::ostream &os) const override;
};

struct Setz final : Instruction {
    std::shared_ptr<Operand> m_operand;

    explicit Setz(const std::shared_ptr<Operand> &operand);

    void print(std::ostream &os) const override;
};

struct Jmp final : Instruction {
    string_t m_label;

    explicit Jmp(const string_t &label);

    void print(std::ostream &os) const override;
};

struct Xor final : Instruction {
    std::shared_ptr<Operand> m_left;
    std::shared_ptr<Operand> m_right;

    explicit Xor(const std::shared_ptr<Operand> &left, const std::shared_ptr<Operand> &right);

    void print(std::ostream &os) const override;
};

