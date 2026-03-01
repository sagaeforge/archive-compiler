#include "kern/codegen/CodeGen.h"
#include <algorithm>
#include <cassert>
#include <stdexcept>

namespace kern {

// System V AMD64 ABI argument registers
static const char* ARG_REGS[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
static constexpr int MAX_ARG_REGS = 6;

CodeGen::CodeGen(std::ostream& out) : out_(out) {}

// Map 64-bit register name to the appropriate width variant
std::string CodeGen::regForWidth(const std::string& reg64, int bits) {
    if (bits == 64) return reg64;

    // rax/eax/ax/al family
    if (reg64 == "rax") { return bits == 32 ? "eax" : bits == 16 ? "ax" : "al"; }
    if (reg64 == "rbx") { return bits == 32 ? "ebx" : bits == 16 ? "bx" : "bl"; }
    if (reg64 == "rcx") { return bits == 32 ? "ecx" : bits == 16 ? "cx" : "cl"; }
    if (reg64 == "rdx") { return bits == 32 ? "edx" : bits == 16 ? "dx" : "dl"; }
    if (reg64 == "rsi") { return bits == 32 ? "esi" : bits == 16 ? "si" : "sil"; }
    if (reg64 == "rdi") { return bits == 32 ? "edi" : bits == 16 ? "di" : "dil"; }
    if (reg64 == "r8")  { return bits == 32 ? "r8d"  : bits == 16 ? "r8w"  : "r8b"; }
    if (reg64 == "r9")  { return bits == 32 ? "r9d"  : bits == 16 ? "r9w"  : "r9b"; }
    if (reg64 == "r10") { return bits == 32 ? "r10d" : bits == 16 ? "r10w" : "r10b"; }
    if (reg64 == "r11") { return bits == 32 ? "r11d" : bits == 16 ? "r11w" : "r11b"; }
    if (reg64 == "r12") { return bits == 32 ? "r12d" : bits == 16 ? "r12w" : "r12b"; }
    if (reg64 == "r13") { return bits == 32 ? "r13d" : bits == 16 ? "r13w" : "r13b"; }
    if (reg64 == "r14") { return bits == 32 ? "r14d" : bits == 16 ? "r14w" : "r14b"; }
    if (reg64 == "r15") { return bits == 32 ? "r15d" : bits == 16 ? "r15w" : "r15b"; }

    return reg64; // fallback
}

void CodeGen::setValueType(ValueId v, IRType t) {
    value_types_[v] = t;
}

IRType CodeGen::getValueType(ValueId v) const {
    auto it = value_types_.find(v);
    if (it != value_types_.end()) return it->second;
    return IRType::I64; // default for backwards compat
}

void CodeGen::initRegs() {
    free_regs_.clear();
    used_callee_saved_.clear();
    value_locs_.clear();
    value_types_.clear();
    stack_offset_ = 0;
    max_stack_ = 0;

    free_regs_ = {"r10", "r11", "rcx", "rdx", "rsi", "rdi"};
}

std::string CodeGen::allocReg(ValueId v) {
    if (!free_regs_.empty()) {
        std::string reg = free_regs_.back();
        free_regs_.pop_back();
        value_locs_[v] = {Location::Reg, reg, 0, getValueType(v)};
        return reg;
    }
    static const char* callee_saved[] = {"rbx", "r12", "r13", "r14", "r15"};
    for (auto& cs : callee_saved) {
        bool in_use = false;
        for (auto& [_, loc] : value_locs_) {
            if (loc.kind == Location::Reg && loc.reg == cs) {
                in_use = true;
                break;
            }
        }
        if (!in_use) {
            if (std::find(used_callee_saved_.begin(), used_callee_saved_.end(), cs)
                == used_callee_saved_.end()) {
                used_callee_saved_.push_back(cs);
            }
            value_locs_[v] = {Location::Reg, cs, 0, getValueType(v)};
            return cs;
        }
    }
    return spillToStack(v, getValueType(v));
}

std::string CodeGen::spillToStack(ValueId v, IRType type) {
    stack_offset_ -= 8; // always 8-byte aligned slots
    if (-stack_offset_ > max_stack_) max_stack_ = -stack_offset_;
    value_locs_[v] = {Location::Stack, "", stack_offset_, type};
    return "qword [rbp" + std::to_string(stack_offset_) + "]";
}

std::string CodeGen::valReg(ValueId v) {
    auto it = value_locs_.find(v);
    if (it == value_locs_.end()) {
        return allocReg(v);
    }
    if (it->second.kind == Location::Reg) {
        return it->second.reg;
    }
    return "qword [rbp" + std::to_string(it->second.stack_offset) + "]";
}

void CodeGen::freeReg(ValueId v) {
    auto it = value_locs_.find(v);
    if (it != value_locs_.end() && it->second.kind == Location::Reg) {
        std::string reg = it->second.reg;
        if (reg != "rbx" && reg != "r12" && reg != "r13" &&
            reg != "r14" && reg != "r15") {
            free_regs_.push_back(reg);
        }
        value_locs_.erase(it);
    }
}

void CodeGen::ensureInReg(ValueId v, const std::string& target_reg) {
    auto it = value_locs_.find(v);
    if (it == value_locs_.end()) return;

    if (it->second.kind == Location::Stack) {
        out_ << "    mov  " << target_reg << ", qword [rbp" << it->second.stack_offset << "]\n";
    } else if (it->second.reg != target_reg) {
        out_ << "    mov  " << target_reg << ", " << it->second.reg << "\n";
    }
}

void CodeGen::collectMergeBlocks(const IRFunction& fn) {
    merge_blocks_.clear();
    for (uint32_t i = 0; i < fn.blocks.size(); ++i) {
        const auto& block = fn.blocks[i];
        if (block.params.size() == 5) {
            MergeInfo mi;
            mi.result_val = block.params[0];
            mi.then_val = block.params[1];
            mi.else_val = block.params[2];
            mi.then_block = static_cast<uint32_t>(block.params[3]);
            mi.else_block = static_cast<uint32_t>(block.params[4]);
            merge_blocks_[i] = mi;
        }
    }
}

void CodeGen::emitModule(const IRModule& mod) {
    out_ << "section .text\n";

    for (const auto& fn : mod.functions) {
        out_ << "global _" << fn.name << "\n";
    }
    out_ << "\n";

    for (const auto& fn : mod.functions) {
        emitFunction(fn);
    }
}

void CodeGen::emitPrologue(const std::string& name) {
    out_ << "_" << name << ":\n";
    out_ << "    push rbp\n";
    out_ << "    mov  rbp, rsp\n";
}

void CodeGen::emitEpilogue() {
    out_ << "    mov  rsp, rbp\n";
    out_ << "    pop  rbp\n";
    out_ << "    ret\n\n";
}

void CodeGen::emitFunction(const IRFunction& fn) {
    initRegs();
    collectMergeBlocks(fn);

    emitPrologue(fn.name);
    out_ << "    push rbx\n";

    // Map parameters — ABI always uses 64-bit registers, store type info
    for (size_t i = 0; i < fn.param_values.size() && i < MAX_ARG_REGS; ++i) {
        IRType pt = (i < fn.param_types.size()) ? fn.param_types[i] : IRType::I64;
        setValueType(fn.param_values[i], pt);
        std::string reg = allocReg(fn.param_values[i]);
        if (reg != ARG_REGS[i]) {
            out_ << "    mov  " << reg << ", " << ARG_REGS[i] << "\n";
        }
    }

    for (uint32_t i = 0; i < fn.blocks.size(); ++i) {
        emitBlock(fn, i);
    }

    out_ << "\n";
}

void CodeGen::emitBlock(const IRFunction& fn, uint32_t block_idx) {
    const auto& block = fn.blocks[block_idx];
    current_block_idx_ = block_idx;

    if (block_idx > 0) {
        out_ << "._" << block.label << ":\n";
    }

    auto merge_it = merge_blocks_.find(block_idx);
    if (merge_it != merge_blocks_.end()) {
        const auto& mi = merge_it->second;
        value_locs_[mi.result_val] = {Location::Reg, "rax", 0, IRType::Unknown};
    }

    for (const auto& instr : block.instrs) {
        emitInstr(fn, instr);
    }
}

void CodeGen::emitInstr(const IRFunction& fn, const IRInstr& instr) {
    // Record type for this value
    if (instr.result != INVALID_VALUE && instr.type != IRType::Unknown) {
        setValueType(instr.result, instr.type);
    }

    switch (instr.op) {
        case IROpcode::ConstInt: {
            std::string reg = allocReg(instr.result);
            out_ << "    mov  " << reg << ", " << instr.imm_value << "\n";
            break;
        }

        case IROpcode::Add: {
            std::string lhs_reg = valReg(instr.operands[0]);
            std::string rhs_reg = valReg(instr.operands[1]);
            std::string dst = allocReg(instr.result);

            if (dst != lhs_reg) {
                out_ << "    mov  " << dst << ", " << lhs_reg << "\n";
            }
            out_ << "    add  " << dst << ", " << rhs_reg << "\n";
            break;
        }

        case IROpcode::Sub: {
            std::string lhs_reg = valReg(instr.operands[0]);
            std::string rhs_reg = valReg(instr.operands[1]);
            std::string dst = allocReg(instr.result);

            if (dst != lhs_reg) {
                out_ << "    mov  " << dst << ", " << lhs_reg << "\n";
            }
            out_ << "    sub  " << dst << ", " << rhs_reg << "\n";
            break;
        }

        case IROpcode::Mul: {
            std::string lhs_reg = valReg(instr.operands[0]);
            std::string rhs_reg = valReg(instr.operands[1]);
            std::string dst = allocReg(instr.result);

            if (dst != lhs_reg) {
                out_ << "    mov  " << dst << ", " << lhs_reg << "\n";
            }
            out_ << "    imul " << dst << ", " << rhs_reg << "\n";
            break;
        }

        case IROpcode::Div: {
            std::string lhs_reg = valReg(instr.operands[0]);
            std::string rhs_reg = valReg(instr.operands[1]);

            // Determine signedness from the operand type
            IRType operand_type = getValueType(instr.operands[0]);
            bool is_signed = irTypeIsSigned(operand_type) ||
                             operand_type == IRType::Unknown ||
                             operand_type == IRType::I64; // default

            out_ << "    mov  rax, " << lhs_reg << "\n";
            if (is_signed) {
                out_ << "    cqo\n";
                out_ << "    idiv " << rhs_reg << "\n";
            } else {
                out_ << "    xor  edx, edx\n";
                out_ << "    div  " << rhs_reg << "\n";
            }
            std::string dst = allocReg(instr.result);
            if (dst != "rax") {
                out_ << "    mov  " << dst << ", rax\n";
            }
            break;
        }

        case IROpcode::ICmpEq:
        case IROpcode::ICmpNe:
        case IROpcode::ICmpLt:
        case IROpcode::ICmpLe:
        case IROpcode::ICmpGt:
        case IROpcode::ICmpGe: {
            std::string lhs_reg = valReg(instr.operands[0]);
            std::string rhs_reg = valReg(instr.operands[1]);

            out_ << "    cmp  " << lhs_reg << ", " << rhs_reg << "\n";

            // Determine signedness of operands for ordered comparisons
            IRType operand_type = getValueType(instr.operands[0]);
            bool is_unsigned = (operand_type == IRType::U8 || operand_type == IRType::U16 ||
                                operand_type == IRType::U32 || operand_type == IRType::U64);

            const char* setcc;
            switch (instr.op) {
                case IROpcode::ICmpEq: setcc = "sete"; break;
                case IROpcode::ICmpNe: setcc = "setne"; break;
                case IROpcode::ICmpLt: setcc = is_unsigned ? "setb"  : "setl"; break;
                case IROpcode::ICmpLe: setcc = is_unsigned ? "setbe" : "setle"; break;
                case IROpcode::ICmpGt: setcc = is_unsigned ? "seta"  : "setg"; break;
                case IROpcode::ICmpGe: setcc = is_unsigned ? "setae" : "setge"; break;
                default: setcc = "sete"; break;
            }

            std::string dst = allocReg(instr.result);
            out_ << "    " << setcc << " al\n";
            out_ << "    movzx " << dst << ", al\n";
            break;
        }

        case IROpcode::Neg:
        case IROpcode::Not:
            break;

        case IROpcode::Branch: {
            auto merge_it = merge_blocks_.find(instr.target_block);
            if (merge_it != merge_blocks_.end()) {
                const auto& mi = merge_it->second;
                ValueId val_to_move;
                if (current_block_idx_ == mi.then_block) {
                    val_to_move = mi.then_val;
                } else {
                    val_to_move = mi.else_val;
                }
                std::string src = valReg(val_to_move);
                if (src != "rax") {
                    out_ << "    mov  rax, " << src << "\n";
                }
            }
            out_ << "    jmp  ._" << fn.blocks[instr.target_block].label << "\n";
            break;
        }

        case IROpcode::CondBranch: {
            std::string cond_reg = valReg(instr.operands[0]);
            out_ << "    test " << cond_reg << ", " << cond_reg << "\n";
            out_ << "    jnz  ._" << fn.blocks[instr.target_block].label << "\n";
            out_ << "    jmp  ._" << fn.blocks[instr.false_block].label << "\n";
            break;
        }

        case IROpcode::Ret: {
            if (!instr.operands.empty()) {
                std::string val = valReg(instr.operands[0]);
                if (val != "rax") {
                    out_ << "    mov  rax, " << val << "\n";
                }
            }
            out_ << "    pop  rbx\n";
            out_ << "    mov  rsp, rbp\n";
            out_ << "    pop  rbp\n";
            out_ << "    ret\n";
            break;
        }

        case IROpcode::Call: {
            std::vector<std::pair<ValueId, std::string>> to_save;
            for (auto& [vid, loc] : value_locs_) {
                if (loc.kind == Location::Reg) {
                    std::string& r = loc.reg;
                    if (r == "r10" || r == "r11" || r == "rcx" || r == "rdx" ||
                        r == "rsi" || r == "rdi") {
                        to_save.push_back({vid, r});
                    }
                }
            }

            for (auto& [vid, reg] : to_save) {
                out_ << "    push " << reg << "\n";
            }

            std::vector<std::string> arg_sources;
            for (size_t i = 0; i < instr.operands.size(); ++i) {
                arg_sources.push_back(valReg(instr.operands[i]));
            }

            for (size_t i = 0; i < instr.operands.size() && i < MAX_ARG_REGS; ++i) {
                if (arg_sources[i] != ARG_REGS[i]) {
                    out_ << "    mov  " << ARG_REGS[i] << ", " << arg_sources[i] << "\n";
                }
            }

            out_ << "    call _" << instr.callee_name << "\n";

            for (auto it = to_save.rbegin(); it != to_save.rend(); ++it) {
                out_ << "    pop  " << it->second << "\n";
            }

            std::string dst = allocReg(instr.result);
            if (dst != "rax") {
                out_ << "    mov  " << dst << ", rax\n";
            }
            break;
        }
    }
}

} // namespace kern
