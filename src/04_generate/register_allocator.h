//
// Created by lambda on 11/22/25.
//

#pragma once

#include "operand.h"

class Compiler;

class RegisterAllocator {
public:
    explicit RegisterAllocator(Compiler &compiler);

public:
    Register allocate(const std::optional<Register> &hint = std::nullopt);
    void free(const Register &reg);
    void restoreIfSpilled(const Register &reg);

    std::optional<Register> getVariableRegister(const string_t &variableName);
    void bindVariable(const string_t &variableName, const Register &reg);
    std::vector<Register> saveCallerSaved(const std::vector<Register> &exclude = {});
    void restoreCallerSaved(const std::vector<Register> &saved);

private:
    Register spillRegister();

private:
    static std::unordered_map<string_t, Register> g_registers;
    std::vector<Register> m_available;
    std::vector<Register> m_inUse;
    std::unordered_map<string_t, Register> m_var2Reg;

    std::unordered_map<Register, std::uint32_t> m_spilled;
    std::uint32_t m_spillOffset;
    Compiler &m_compiler;
};
