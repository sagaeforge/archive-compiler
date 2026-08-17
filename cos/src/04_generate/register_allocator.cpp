//
// Created by lambda on 11/22/25.
//

#include "register_allocator.h"

#include "compiler.h"
#include "instruction.h"

std::unordered_map<string_t, Register> RegisterAllocator::g_registers = {
    {"rax", Register("rax"),},
    {"rcx", Register("rcx"),},
    {"rdx", Register("rdx"),},
    {"r8", Register("r8"),},
    {"r9", Register("r9"),},
    {"r10", Register("r10"),},
    {"r11", Register("r11"),},
    {"rbx", Register("rbx"),},
    {"r12", Register("r12"),},
    {"r13", Register("r13"),},
    {"r14", Register("r14"),},
    {"r15", Register("r15"),},
};

RegisterAllocator::RegisterAllocator(Compiler &compiler) : m_spillOffset(0), m_compiler(compiler) {
    m_available = {
        g_registers["rax"],
        g_registers["rcx"],
        g_registers["rdx"],
        g_registers["r8"],
        g_registers["r9"],
        g_registers["r10"],
        g_registers["r11"],
        g_registers["rbx"],
        g_registers["r12"],
        g_registers["r13"],
        g_registers["r14"],
        g_registers["r15"],
    };
}

Register RegisterAllocator::allocate(const std::optional<Register> &hint) {
    Register reg;
    if (hint.has_value() && vector_contains(m_available, hint.value())) {
        reg = hint.value();
        vector_remove(m_available, reg);
    } else if (!m_available.empty()) {
        reg = m_available.front();
        m_available.erase(m_available.begin());
    } else {
        reg = spillRegister();
    }

    m_inUse.push_back(reg);
    return reg;
}

void RegisterAllocator::free(const Register &reg) {
    if (vector_contains(m_inUse, reg)) {
        vector_remove(m_inUse, reg);
        if (!vector_contains(m_available, reg)) {
            m_available.push_back(reg);
        }
    }
}

void RegisterAllocator::restoreIfSpilled(const Register &reg) {
    if (const auto it = m_spilled.find(reg); it != m_spilled.end()) {
        m_compiler.emitComment("RESTORE: " + reg.m_name + " from stack");
        m_compiler.emit(std::make_shared<Pop>(std::make_shared<Register>(reg)));
        m_spilled.erase(it);
    }
}

std::optional<Register> RegisterAllocator::getVariableRegister(const string_t &variableName) {
    if (const auto it = m_var2Reg.find(variableName); it != m_var2Reg.end()) {
        return it->second;
    }
    return std::nullopt;
}

void RegisterAllocator::bindVariable(const string_t &variableName, const Register &reg) {
    m_var2Reg[variableName] = reg;
}

std::vector<Register> RegisterAllocator::saveCallerSaved(const std::vector<Register> &exclude) {
    std::vector<Register> caller_saved = {
        g_registers["rax"],
        g_registers["rcx"],
        g_registers["rdx"],
        g_registers["r8"],
        g_registers["r9"],
        g_registers["r10"],
        g_registers["r11"],
    };

    std::vector<Register> saved;
    for (const auto &reg: caller_saved) {
        if (vector_contains(m_inUse, reg) && !vector_contains(exclude, reg)) {
            m_compiler.emit(std::make_shared<Push>(std::make_shared<Register>(reg)));
            m_compiler.emitComment("save caller-saved");
            saved.push_back(reg);
        }
    }

    return saved;
}

void RegisterAllocator::restoreCallerSaved(const std::vector<Register> &saved) {
    for (auto it = saved.rbegin(); it != saved.rend(); ++it) {
        m_compiler.emit(std::make_shared<Pop>(std::make_shared<Register>(*it)));
        m_compiler.emitComment("restore caller-saved");
    }
}

Register RegisterAllocator::spillRegister() {
    if (m_inUse.empty()) {
        throw std::runtime_error("No registers to spill");
    }

    auto &victim = m_inUse.front();
    m_spillOffset += 8;
    m_spilled[victim] = m_spillOffset;

    m_compiler.emitComment("SPILL:" + victim.m_name + "to stack");
    m_compiler.emit(std::make_shared<Push>(std::make_shared<Register>(victim)));

    vector_remove(m_inUse, victim);
    return victim;
}
