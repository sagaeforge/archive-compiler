//
// Created by lambda on 11/22/25.
//

#include "operand.h"

Register::Register() {
}

Register::Register(string_t name) : m_name(std::move(name)) {
}

Register::Register(const Register &reg) : m_name(reg.m_name) {
}

Register &Register::operator=(const Register &rhs) {
    m_name = rhs.m_name;
    return *this;
}

void Register::print(std::ostream &os) const {
    os << m_name;
}

Memory::Memory(const std::shared_ptr<Register> &reg, std::uint32_t offset) : m_reg(reg), m_label{},
                                                                             m_offset(offset) {
}

Memory::Memory(const string_t &label) : m_label(label), m_offset(0) {
}

void Memory::print(std::ostream &os) const {
    if (m_label.has_value()) {
        os << m_label.value();
        return;
    }

    os << "[" << m_reg.get() << "-" << m_offset << "]";
}

Immutable::Immutable(std::uint64_t value) : m_value(value) {
}

void Immutable::print(std::ostream &os) const {
    os << m_value;
}

bool Register::operator==(const Register &rhs) const {
    return this->m_name == rhs.m_name;
}

bool Register::operator!=(const Register &rhs) const {
    return rhs.m_name != m_name;
}
