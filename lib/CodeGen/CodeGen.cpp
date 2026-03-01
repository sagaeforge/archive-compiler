#include "kern/codegen/CodeGen.h"
#include <algorithm>
#include <cassert>
#include <stdexcept>

namespace kern {

// System V AMD64 ABI argument registers
static const char* ARG_REGS[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
static constexpr int MAX_ARG_REGS = 6;

CodeGen::CodeGen(std::ostream& out) : out_(out) {}

void CodeGen::initRegs() {
    free_regs_.clear();
    used_callee_saved_.clear();
    value_locs_.clear();
    stack_offset_ = 0;
    max_stack_ = 0;

    // Caller-saved registers (prefer these for temporaries)
    // Exclude rax (return value), rdi-r9 (args — may be reused after setup)
    free_regs_ = {"r10", "r11", "rcx", "rdx", "rsi", "rdi"};
    // Callee-saved registers (use when caller-saved are exhausted)
    // rbx, r12-r15 are callee-saved
}

std::string CodeGen::allocReg(ValueId v) {
    if (!free_regs_.empty()) {
        std::string reg = free_regs_.back();
        free_regs_.pop_back();
        value_locs_[v] = {Location::Reg, reg, 0};
        return reg;
    }
    // Need to spill — use callee-saved
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
            value_locs_[v] = {Location::Reg, cs, 0};
            return cs;
        }
    }
    // All registers in use — spill to stack
    return spillToStack(v);
}

std::string CodeGen::spillToStack(ValueId v) {
    stack_offset_ -= 8;
    if (-stack_offset_ > max_stack_) max_stack_ = -stack_offset_;
    value_locs_[v] = {Location::Stack, "", stack_offset_};
    // Return a memory operand string
    return "qword [rbp" + std::to_string(stack_offset_) + "]";
}

std::string CodeGen::valReg(ValueId v) {
    auto it = value_locs_.find(v);
    if (it == value_locs_.end()) {
        // Value not found — this can happen with merge values
        // Allocate a register for it
        return allocReg(v);
    }
    if (it->second.kind == Location::Reg) {
        return it->second.reg;
    }
    // On stack — load to rax temporarily
    return "qword [rbp" + std::to_string(it->second.stack_offset) + "]";
}

void CodeGen::freeReg(ValueId v) {
    auto it = value_locs_.find(v);
    if (it != value_locs_.end() && it->second.kind == Location::Reg) {
        std::string reg = it->second.reg;
        // Only free caller-saved regs back to the pool
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

    // Declare all functions as global
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

    // We'll do a two-pass approach for the function:
    // For M1 simplicity, we take a direct approach:
    // - Each function parameter gets mapped from arg registers
    // - We use a straightforward translation

    emitPrologue(fn.name);

    // Save callee-saved registers we might use
    // We'll do this after we know which ones we need — for now, always save rbx
    out_ << "    push rbx\n";

    // Map parameters to their arg register locations
    for (size_t i = 0; i < fn.param_values.size() && i < MAX_ARG_REGS; ++i) {
        // Move arg from ABI register to an allocated register
        std::string reg = allocReg(fn.param_values[i]);
        if (reg != ARG_REGS[i]) {
            out_ << "    mov  " << reg << ", " << ARG_REGS[i] << "\n";
        }
    }

    // Emit all blocks
    for (uint32_t i = 0; i < fn.blocks.size(); ++i) {
        emitBlock(fn, i);
    }

    out_ << "\n";
}

void CodeGen::emitBlock(const IRFunction& fn, uint32_t block_idx) {
    const auto& block = fn.blocks[block_idx];
    current_block_idx_ = block_idx;

    // Emit label (skip for entry block = block 0)
    if (block_idx > 0) {
        out_ << "._" << block.label << ":\n";
    }

    // Handle merge block — set up the result register
    auto merge_it = merge_blocks_.find(block_idx);
    if (merge_it != merge_blocks_.end()) {
        // The merge value is already set by the branch blocks
        // Just allocate a location for it — it's in rax by convention
        // Actually, we handle this by having the then/else blocks
        // move their values into the merge register
        const auto& mi = merge_it->second;
        // The result value needs to be accessible
        value_locs_[mi.result_val] = {Location::Reg, "rax", 0};
    }

    for (const auto& instr : block.instrs) {
        emitInstr(fn, instr);
    }
}

void CodeGen::emitInstr(const IRFunction& fn, const IRInstr& instr) {
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
            // idiv uses rax:rdx / src → rax
            std::string lhs_reg = valReg(instr.operands[0]);
            std::string rhs_reg = valReg(instr.operands[1]);

            out_ << "    mov  rax, " << lhs_reg << "\n";
            out_ << "    cqo\n"; // sign-extend rax into rdx:rax
            if (rhs_reg.find("[") != std::string::npos) {
                out_ << "    idiv " << rhs_reg << "\n";
            } else {
                out_ << "    idiv " << rhs_reg << "\n";
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

            const char* setcc;
            switch (instr.op) {
                case IROpcode::ICmpEq: setcc = "sete"; break;
                case IROpcode::ICmpNe: setcc = "setne"; break;
                case IROpcode::ICmpLt: setcc = "setl"; break;
                case IROpcode::ICmpLe: setcc = "setle"; break;
                case IROpcode::ICmpGt: setcc = "setg"; break;
                case IROpcode::ICmpGe: setcc = "setge"; break;
                default: setcc = "sete"; break;
            }

            std::string dst = allocReg(instr.result);
            // setcc only sets the low byte, so zero-extend
            out_ << "    " << setcc << " al\n";
            out_ << "    movzx " << dst << ", al\n";
            break;
        }

        case IROpcode::Neg:
        case IROpcode::Not:
            // Handled in IR via Sub
            break;

        case IROpcode::Branch: {
            // If branching to a merge block, move the correct value into rax
            auto merge_it = merge_blocks_.find(instr.target_block);
            if (merge_it != merge_blocks_.end()) {
                const auto& mi = merge_it->second;
                // Determine which value to put in rax based on current block
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
            // Restore callee-saved and return
            out_ << "    pop  rbx\n";
            out_ << "    mov  rsp, rbp\n";
            out_ << "    pop  rbp\n";
            out_ << "    ret\n";
            break;
        }

        case IROpcode::Call: {
            // Save any caller-saved registers we care about
            // Push current live values that are in caller-saved regs
            std::vector<std::pair<ValueId, std::string>> to_save;
            for (auto& [vid, loc] : value_locs_) {
                if (loc.kind == Location::Reg) {
                    // Save if it's a caller-saved reg we might need later
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

            // Set up arguments in ABI registers
            // First, collect arg values into a temporary list to avoid clobbering
            std::vector<std::string> arg_sources;
            for (size_t i = 0; i < instr.operands.size(); ++i) {
                arg_sources.push_back(valReg(instr.operands[i]));
            }

            for (size_t i = 0; i < instr.operands.size() && i < MAX_ARG_REGS; ++i) {
                if (arg_sources[i] != ARG_REGS[i]) {
                    out_ << "    mov  " << ARG_REGS[i] << ", " << arg_sources[i] << "\n";
                }
            }

            // Align stack to 16 bytes before call
            // The push rbp + push rbx + N pushes need to result in 16-byte alignment
            // We handle this by padding if needed
            // For simplicity in M1, we rely on the pushes being balanced

            out_ << "    call _" << instr.callee_name << "\n";

            // Restore saved registers (reverse order)
            for (auto it = to_save.rbegin(); it != to_save.rend(); ++it) {
                out_ << "    pop  " << it->second << "\n";
            }

            // Result is in rax
            std::string dst = allocReg(instr.result);
            if (dst != "rax") {
                out_ << "    mov  " << dst << ", rax\n";
            }
            break;
        }
    }
}

} // namespace kern
