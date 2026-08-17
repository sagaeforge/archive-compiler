//
// Created by lambda on 11/21/25.
//

#pragma once
#include <utility>

#include "common.h"
#include "00_core/comparable.h"
#include "00_core/printable.hpp"

class Operand : public printable {
};

struct Register final : Operand {
    string_t m_name; // rax, rdi 등등.

    explicit Register();
    explicit Register(string_t name);

    Register(const Register &reg);

    bool operator==(const Register &) const;
    bool operator!=(const Register &) const;

    Register &operator=(const Register &rhs);

    void print(std::ostream &os) const override;
};

struct Memory final : Operand {
    // [register - offset]
    std::shared_ptr<Register> m_reg;
    std::optional<string_t> m_label;
    std::uint32_t m_offset = 0;

    explicit Memory(const std::shared_ptr<Register> &reg, std::uint32_t offset);

    explicit Memory(const string_t &label);

    void print(std::ostream &os) const override;
};

struct Immutable : Operand {
    // 1 ~ 'a'
    std::uint64_t m_value = 0;

    explicit Immutable(std::uint64_t value);

    void print(std::ostream &os) const override;
};


template<>
struct std::hash<Register> {
    size_t operator()(const Register &r) const noexcept {
        return hash<string_t>{}(r.m_name);
    }
};

template<>
struct std::hash<Memory> {
    size_t operator()(const Memory &m) const noexcept {
        size_t h1 = m.m_reg ? hash<Register>{}(*m.m_reg) : 0;
        size_t h2 = hash<uint32_t>{}(m.m_offset);
        return h1 ^ (h2 << 1);
    }
};

template<>
struct std::hash<Immutable> {
    size_t operator()(const Immutable &v) const noexcept {
        return hash<uint64_t>{}(v.m_value);
    }
};
