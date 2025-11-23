//
// Created by lambda on 11/23/25.
//

#include "instruction.h"

Move::Move(const std::shared_ptr<Operand> &left, const std::shared_ptr<Operand> &right) : m_left(left),
    m_right(right) {
}

Move::Move(const string_t &wordSize, const std::shared_ptr<Operand> &left,
           const std::shared_ptr<Operand> &right) : m_wordSize(wordSize), m_left(left), m_right(right) {
}

void Move::print(std::ostream &os) const {
    os << "mov " << this->m_wordSize << this->m_left << ", " << this->m_right;
}

Push::Push(const std::shared_ptr<Operand> &operand) : m_operand(operand) {
}

void Push::print(std::ostream &os) const {
    os << "push " << this->m_operand;
}

Add::Add(const std::shared_ptr<Operand> &left, const std::shared_ptr<Operand> &right) : m_left(left),
    m_right(right) {
}

void Add::print(std::ostream &os) const {
    os << "add " << this->m_left << ", " << this->m_right;
}

Sub::Sub(const std::shared_ptr<Operand> &left, const std::shared_ptr<Operand> &right) : m_left(left),
    m_right(right) {
}

void Sub::print(std::ostream &os) const {
    os << "sub " << this->m_left << ", " << this->m_right;
}

Cmp::Cmp(const std::shared_ptr<Operand> &left, const std::shared_ptr<Operand> &right) : m_left(left),
    m_right(right) {
}

void Cmp::print(std::ostream &os) const {
    os << "cmp " << this->m_left << ", " << this->m_right;
}

Movzx::Movzx(const std::shared_ptr<Operand> &left, const std::shared_ptr<Operand> &right) : m_left(left),
    m_right(right) {
}

void Movzx::print(std::ostream &os) const {
    os << "movzx " << this->m_left << ", " << this->m_right;
}

Je::Je(const string_t &label) : m_label(label) {
}

void Je::print(std::ostream &os) const {
    os << "je " << this->m_label;
}

Pop::Pop(const std::shared_ptr<Operand> &operand) : m_operand(operand) {
}

void Pop::print(std::ostream &os) const {
    os << "pop " << this->m_operand;
}

void Ret::print(std::ostream &os) const {
    os << "ret";
}

Call::Call(const string_t &label) : m_label(label) {
}

void Call::print(std::ostream &os) const {
    os << "call " << this->m_label;
}

void Syscall::print(std::ostream &os) const {
    os << "syscall";
}

Lea::Lea(const std::shared_ptr<Operand> &left, const std::shared_ptr<Operand> &right) : m_left(left),
    m_right(right) {
}

void Lea::print(std::ostream &os) const {
    os << "lea " << this->m_left << ", " << this->m_right;
}

Mul::Mul(const std::shared_ptr<Operand> &left, const std::shared_ptr<Operand> &right) : m_left(left),
    m_right(right) {
}

void Mul::print(std::ostream &os) const {
    os << "mul " << this->m_left << ", " << this->m_right;
}

IDiv::IDiv(const std::shared_ptr<Operand> &operand) : m_operand(operand) {
}

void IDiv::print(std::ostream &os) const {
    os << "idiv " << this->m_operand;
}

void Cqo::print(std::ostream &os) const {
    os << "cqo";
}

Sete::Sete(const std::shared_ptr<Operand> &operand) : m_operand(operand) {
}

void Sete::print(std::ostream &os) const {
    os << "sete " << this->m_operand;
}

Setne::Setne(const std::shared_ptr<Operand> &operand) : m_operand(operand) {
}

void Setne::print(std::ostream &os) const {
    os << "setne " << this->m_operand;
}

Setl::Setl(const std::shared_ptr<Operand> &operand) : m_operand(operand) {
}

void Setl::print(std::ostream &os) const {
    os << "setl " << this->m_operand;
}

Setle::Setle(const std::shared_ptr<Operand> &operand) : m_operand(operand) {
}

void Setle::print(std::ostream &os) const {
    os << "setle " << this->m_operand;
}

Setg::Setg(const std::shared_ptr<Operand> &operand) : m_operand(operand) {
}

void Setg::print(std::ostream &os) const {
    os << "setg " << this->m_operand;
}

Setge::Setge(const std::shared_ptr<Operand> &operand) : m_operand(operand) {
}

void Setge::print(std::ostream &os) const {
    os << "setge " << this->m_operand;
}

Neg::Neg(const std::shared_ptr<Operand> &operand) : m_operand(operand) {
}

void Neg::print(std::ostream &os) const {
    os << "neg " << this->m_operand;
}

Test::Test(const std::shared_ptr<Operand> &left, const std::shared_ptr<Operand> &right) : m_left(left),
    m_right(right) {
}

void Test::print(std::ostream &os) const {
    os << "test " << this->m_left << ", " << this->m_right;
}

Setz::Setz(const std::shared_ptr<Operand> &operand) : m_operand(operand) {
}

void Setz::print(std::ostream &os) const {
    os << "setz " << this->m_operand;
}

Jmp::Jmp(const string_t &label) : m_label(label) {
}

void Jmp::print(std::ostream &os) const {
    os << "jmp " << this->m_label;
}

Xor::Xor(const std::shared_ptr<Operand> &left, const std::shared_ptr<Operand> &right) : m_left(left), m_right(right) {
}

void Xor::print(std::ostream &os) const {
    os << "xor " << this->m_left << ", " << this->m_right;
}
