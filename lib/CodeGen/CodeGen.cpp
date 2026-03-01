#include "kern/codegen/CodeGen.h"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <sstream>

namespace kern {

// System V AMD64 ABI argument registers (integer)
static const char* ARG_REGS[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
static constexpr int MAX_ARG_REGS = 6;
// System V AMD64 ABI argument registers (float)
static const char* XMM_ARG_REGS[] = {"xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7"};
static constexpr int MAX_XMM_ARG_REGS = 8;

static bool isMemOperand(const std::string& s) {
    return s.find('[') != std::string::npos;
}

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
    free_xmm_regs_.clear();
    value_locs_.clear();
    value_types_.clear();
    // Reserve space for callee-saved registers (rbx, r12-r15 = 5 regs × 8 bytes)
    // that may be push'd after `mov rbp, rsp` in the prologue.
    // Spill slots must not overlap with these.
    static constexpr int CALLEE_SAVED_RESERVE = 5 * 8; // 40 bytes
    stack_offset_ = -CALLEE_SAVED_RESERVE;
    max_stack_ = CALLEE_SAVED_RESERVE;

    free_regs_ = {"r10", "r11", "rcx", "rdx", "rsi", "rdi"};
    free_xmm_regs_ = {"xmm15", "xmm14", "xmm13", "xmm12", "xmm11", "xmm10", "xmm9", "xmm8"};
    tail_call_sites_.clear();
    tail_call_counter_ = 0;
    ret_epilogue_labels_.clear();
    ret_epilogue_counter_ = 0;
}

std::string CodeGen::addFloatConst(double value, bool is_f32) {
    std::string label = "_fc" + std::to_string(float_const_counter_++);
    float_consts_.push_back({label, value, is_f32});
    return label;
}

std::string CodeGen::allocXmmReg(ValueId v) {
    if (!free_xmm_regs_.empty()) {
        std::string reg = free_xmm_regs_.back();
        free_xmm_regs_.pop_back();
        value_locs_[v] = {Location::XmmReg, reg, 0, getValueType(v)};
        return reg;
    }
    // Spill to stack (store from xmm to memory)
    stack_offset_ -= 8;
    if (-stack_offset_ > max_stack_) max_stack_ = -stack_offset_;
    value_locs_[v] = {Location::Stack, "", stack_offset_, getValueType(v)};
    return "qword [rbp" + std::to_string(stack_offset_) + "]";
}

std::string CodeGen::valXmmReg(ValueId v) {
    auto it = value_locs_.find(v);
    if (it == value_locs_.end()) {
        return allocXmmReg(v);
    }
    if (it->second.kind == Location::XmmReg) {
        return it->second.reg;
    }
    if (it->second.kind == Location::Stack) {
        return "qword [rbp" + std::to_string(it->second.stack_offset) + "]";
    }
    return it->second.reg;
}

void CodeGen::freeXmmReg(ValueId v) {
    auto it = value_locs_.find(v);
    if (it != value_locs_.end() && it->second.kind == Location::XmmReg) {
        free_xmm_regs_.push_back(it->second.reg);
        value_locs_.erase(it);
    }
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
        out() << "    mov  " << target_reg << ", qword [rbp" << it->second.stack_offset << "]\n";
    } else if (it->second.reg != target_reg) {
        out() << "    mov  " << target_reg << ", " << it->second.reg << "\n";
    }
}

void CodeGen::collectMergeBlocks(const IRFunction& fn) {
    merge_blocks_.clear();
    for (uint32_t i = 0; i < fn.blocks.size(); ++i) {
        const auto& block = fn.blocks[i];
        if (block.is_merge && block.params.size() >= 5) {
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
    float_consts_.clear();
    float_const_counter_ = 0;

    out() << "section .text\n";

    for (const auto& fn : mod.functions) {
        out() << "global _" << fn.name << "\n";
    }
    out() << "\n";

    for (const auto& fn : mod.functions) {
        emitFunction(fn);
    }

    // Emit float constants in .rodata
    if (!float_consts_.empty()) {
        out() << "\nsection .rodata\n";
        out() << "align 8\n";
        for (const auto& fc : float_consts_) {
            if (fc.is_f32) {
                float f = static_cast<float>(fc.value);
                uint32_t bits;
                std::memcpy(&bits, &f, sizeof(bits));
                out() << fc.label << ": dd 0x" << std::hex << bits << std::dec << "\n";
            } else {
                uint64_t bits;
                std::memcpy(&bits, &fc.value, sizeof(bits));
                out() << fc.label << ": dq 0x" << std::hex << bits << std::dec << "\n";
            }
        }
    }
}

void CodeGen::emitPrologue(const std::string& name) {
    out() << "_" << name << ":\n";
    out() << "    push rbp\n";
    out() << "    mov  rbp, rsp\n";
}

void CodeGen::emitFunction(const IRFunction& fn) {
    initRegs();
    collectMergeBlocks(fn);

    // Emit body to a buffer first so we know max_stack_ before writing prologue
    std::ostringstream body_buf;
    std::ostream& saved_out = out_;
    out_ref_ = &body_buf;

    // Map parameters — split into GPR and XMM paths per System V ABI
    int gpr_idx = 0;
    int xmm_idx = 0;
    for (size_t i = 0; i < fn.param_values.size(); ++i) {
        IRType pt = (i < fn.param_types.size()) ? fn.param_types[i] : IRType::I64;
        setValueType(fn.param_values[i], pt);
        if (irTypeIsFloat(pt)) {
            if (xmm_idx < MAX_XMM_ARG_REGS) {
                std::string reg = allocXmmReg(fn.param_values[i]);
                std::string suffix = (pt == IRType::F32) ? "ss" : "sd";
                if (reg != XMM_ARG_REGS[xmm_idx]) {
                    out() << "    movs" << suffix.back() << " " << reg << ", " << XMM_ARG_REGS[xmm_idx] << "\n";
                }
                xmm_idx++;
            }
        } else {
            if (gpr_idx < MAX_ARG_REGS) {
                std::string reg = allocReg(fn.param_values[i]);
                if (reg != ARG_REGS[gpr_idx]) {
                    out() << "    mov  " << reg << ", " << ARG_REGS[gpr_idx] << "\n";
                }
                gpr_idx++;
            }
        }
    }

    for (uint32_t i = 0; i < fn.blocks.size(); ++i) {
        emitBlock(fn, i);
    }

    // Deferred tail call epilogues (used_callee_saved_ is now finalized)
    for (auto& site : tail_call_sites_) {
        out() << site.label << ":\n";
        int n_callee = static_cast<int>(used_callee_saved_.size());
        if (n_callee > 0) {
            out() << "    lea  rsp, [rbp-" << (n_callee * 8) << "]\n";
        } else {
            out() << "    mov  rsp, rbp\n";
        }
        for (auto it = used_callee_saved_.rbegin();
             it != used_callee_saved_.rend(); ++it) {
            out() << "    pop  " << *it << "\n";
        }
        out() << "    pop  rbp\n";
        out() << "    jmp  _" << site.callee << "\n";
    }

    // Deferred ret epilogues (used_callee_saved_ is now finalized)
    for (auto& label : ret_epilogue_labels_) {
        out() << label << ":\n";
        int n_callee = static_cast<int>(used_callee_saved_.size());
        if (n_callee > 0) {
            out() << "    lea  rsp, [rbp-" << (n_callee * 8) << "]\n";
        } else {
            out() << "    mov  rsp, rbp\n";
        }
        for (auto it = used_callee_saved_.rbegin();
             it != used_callee_saved_.rend(); ++it) {
            out() << "    pop  " << *it << "\n";
        }
        out() << "    pop  rbp\n";
        out() << "    ret\n";
    }

    // Now write prologue with correct stack reservation to the real output
    out_ref_ = &saved_out;
    emitPrologue(fn.name);

    // Save all callee-saved registers that were actually used
    for (const auto& cs : used_callee_saved_) {
        out() << "    push " << cs << "\n";
    }

    // Align stack: push rbp (1) + N callee-saved pushes.
    // If total pushes is even, RSP is 16-byte aligned; if odd, need 8-byte pad.
    int total_pushes = 1 + static_cast<int>(used_callee_saved_.size()); // rbp + callee-saved
    int32_t frame_size = max_stack_;
    if (total_pushes % 2 == 1) {
        frame_size += 8; // pad to maintain 16-byte alignment
    }
    if (frame_size > 0) {
        frame_size = (frame_size + 15) & ~15;
        out() << "    sub  rsp, " << frame_size << "\n";
    }

    // Append the body
    out() << body_buf.str();
    out() << "\n";
}

void CodeGen::emitBlock(const IRFunction& fn, uint32_t block_idx) {
    const auto& block = fn.blocks[block_idx];
    current_block_idx_ = block_idx;

    if (block_idx > 0) {
        out() << "._" << block.label << ":\n";
    }

    auto merge_it = merge_blocks_.find(block_idx);
    if (merge_it != merge_blocks_.end()) {
        const auto& mi = merge_it->second;
        IRType merge_type = getValueType(mi.then_val);
        if (irTypeIsFloat(merge_type)) {
            value_locs_[mi.result_val] = {Location::XmmReg, "xmm0", 0, merge_type};
        } else {
            value_locs_[mi.result_val] = {Location::Reg, "rax", 0, IRType::Unknown};
        }
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
            out() << "    mov  " << reg << ", " << instr.imm_value << "\n";
            break;
        }

        case IROpcode::Add: {
            std::string lhs_reg = valReg(instr.operands[0]);
            std::string rhs_reg = valReg(instr.operands[1]);
            std::string dst = allocReg(instr.result);

            if (isMemOperand(dst)) {
                out() << "    mov  rax, " << lhs_reg << "\n";
                if (isMemOperand(rhs_reg)) {
                    out() << "    add  rax, " << rhs_reg << "\n";
                } else {
                    out() << "    add  rax, " << rhs_reg << "\n";
                }
                out() << "    mov  " << dst << ", rax\n";
            } else {
                if (dst != lhs_reg) {
                    out() << "    mov  " << dst << ", " << lhs_reg << "\n";
                }
                out() << "    add  " << dst << ", " << rhs_reg << "\n";
            }
            break;
        }

        case IROpcode::Sub: {
            std::string lhs_reg = valReg(instr.operands[0]);
            std::string rhs_reg = valReg(instr.operands[1]);
            std::string dst = allocReg(instr.result);

            if (isMemOperand(dst)) {
                out() << "    mov  rax, " << lhs_reg << "\n";
                if (isMemOperand(rhs_reg)) {
                    out() << "    sub  rax, " << rhs_reg << "\n";
                } else {
                    out() << "    sub  rax, " << rhs_reg << "\n";
                }
                out() << "    mov  " << dst << ", rax\n";
            } else {
                if (dst != lhs_reg) {
                    out() << "    mov  " << dst << ", " << lhs_reg << "\n";
                }
                out() << "    sub  " << dst << ", " << rhs_reg << "\n";
            }
            break;
        }

        case IROpcode::Mul: {
            std::string lhs_reg = valReg(instr.operands[0]);
            std::string rhs_reg = valReg(instr.operands[1]);
            std::string dst = allocReg(instr.result);

            if (isMemOperand(dst)) {
                out() << "    mov  rax, " << lhs_reg << "\n";
                out() << "    imul rax, " << rhs_reg << "\n";
                out() << "    mov  " << dst << ", rax\n";
            } else {
                if (dst != lhs_reg) {
                    out() << "    mov  " << dst << ", " << lhs_reg << "\n";
                }
                out() << "    imul " << dst << ", " << rhs_reg << "\n";
            }
            break;
        }

        case IROpcode::Div: {
            std::string lhs_reg = valReg(instr.operands[0]);
            std::string rhs_reg = valReg(instr.operands[1]);

            // Determine signedness and width from the operand type
            IRType operand_type = getValueType(instr.operands[0]);
            bool is_signed = irTypeIsSigned(operand_type) ||
                             operand_type == IRType::Unknown ||
                             operand_type == IRType::I64; // default
            int bits = irTypeBitWidth(operand_type);

            std::string ax = regForWidth("rax", bits);
            std::string divisor_reg = (bits < 64) ? regForWidth(rhs_reg, bits) : rhs_reg;

            out() << "    mov  rax, " << lhs_reg << "\n";
            if (is_signed) {
                if (bits <= 32) {
                    out() << "    cdq\n";
                } else {
                    out() << "    cqo\n";
                }
                out() << "    idiv " << (bits < 64 ? divisor_reg : rhs_reg) << "\n";
            } else {
                out() << "    xor  edx, edx\n";
                out() << "    div  " << (bits < 64 ? divisor_reg : rhs_reg) << "\n";
            }
            std::string dst = allocReg(instr.result);
            if (dst != "rax") {
                out() << "    mov  " << dst << ", rax\n";
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

            // Allocate dst first, xor to clear it BEFORE cmp (xor clobbers flags)
            std::string dst = allocReg(instr.result);

            // If rhs is memory and lhs is also memory, load rhs into rax first
            std::string cmp_rhs = rhs_reg;
            if (isMemOperand(lhs_reg) && isMemOperand(rhs_reg)) {
                out() << "    mov  rax, " << rhs_reg << "\n";
                cmp_rhs = "rax";
            }

            if (isMemOperand(dst)) {
                // dst is memory — can't use xor/setcc on it directly
                // Use: cmp lhs, rhs; setcc al; movzx eax, al; mov dst, rax
                out() << "    cmp  " << lhs_reg << ", " << cmp_rhs << "\n";
                out() << "    " << setcc << " al\n";
                out() << "    movzx rax, al\n";
                out() << "    mov  " << dst << ", rax\n";
            } else {
                std::string dst32 = regForWidth(dst, 32);
                std::string dst8 = regForWidth(dst, 8);
                out() << "    xor  " << dst32 << ", " << dst32 << "\n";
                out() << "    cmp  " << lhs_reg << ", " << cmp_rhs << "\n";
                out() << "    " << setcc << " " << dst8 << "\n";
            }
            break;
        }

        case IROpcode::Neg: {
            std::string src = valReg(instr.operands[0]);
            std::string dst = allocReg(instr.result);
            if (isMemOperand(dst)) {
                out() << "    mov  rax, " << src << "\n";
                out() << "    neg  rax\n";
                out() << "    mov  " << dst << ", rax\n";
            } else {
                if (dst != src) {
                    out() << "    mov  " << dst << ", " << src << "\n";
                }
                out() << "    neg  " << dst << "\n";
            }
            break;
        }

        case IROpcode::Not: {
            std::string src = valReg(instr.operands[0]);
            std::string dst = allocReg(instr.result);
            if (isMemOperand(dst)) {
                out() << "    mov  rax, " << src << "\n";
                out() << "    xor  rax, 1\n";
                out() << "    mov  " << dst << ", rax\n";
            } else {
                if (dst != src) {
                    out() << "    mov  " << dst << ", " << src << "\n";
                }
                out() << "    xor  " << dst << ", 1\n";
            }
            break;
        }

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
                IRType val_type = getValueType(val_to_move);
                if (irTypeIsFloat(val_type)) {
                    std::string src = valXmmReg(val_to_move);
                    std::string suffix = (val_type == IRType::F32) ? "ss" : "sd";
                    if (src != "xmm0") {
                        out() << "    movs" << suffix[1] << " xmm0, " << src << "\n";
                    }
                } else {
                    std::string src = valReg(val_to_move);
                    if (src != "rax") {
                        out() << "    mov  rax, " << src << "\n";
                    }
                }
            }
            out() << "    jmp  ._" << fn.blocks[instr.target_block].label << "\n";
            break;
        }

        case IROpcode::CondBranch: {
            std::string cond_reg = valReg(instr.operands[0]);
            if (isMemOperand(cond_reg)) {
                out() << "    mov  rax, " << cond_reg << "\n";
                out() << "    test rax, rax\n";
            } else {
                out() << "    test " << cond_reg << ", " << cond_reg << "\n";
            }
            out() << "    jnz  ._" << fn.blocks[instr.target_block].label << "\n";
            out() << "    jmp  ._" << fn.blocks[instr.false_block].label << "\n";
            break;
        }

        case IROpcode::Ret: {
            if (!instr.operands.empty()) {
                IRType ret_type = getValueType(instr.operands[0]);
                if (irTypeIsFloat(ret_type)) {
                    std::string val = valXmmReg(instr.operands[0]);
                    std::string suffix = (ret_type == IRType::F32) ? "ss" : "sd";
                    if (val != "xmm0") {
                        out() << "    movs" << suffix[1] << " xmm0, " << val << "\n";
                    }
                } else {
                    std::string val = valReg(instr.operands[0]);
                    if (val != "rax") {
                        out() << "    mov  rax, " << val << "\n";
                    }
                }
            }
            // Defer epilogue to after all blocks (used_callee_saved_ not yet finalized)
            std::string label = "._ret_" + std::to_string(ret_epilogue_counter_++);
            ret_epilogue_labels_.push_back(label);
            out() << "    jmp  " << label << "\n";
            break;
        }

        case IROpcode::ConstFloat: {
            bool is_f32 = (instr.type == IRType::F32);
            std::string label = addFloatConst(instr.imm_float, is_f32);
            std::string dst = allocXmmReg(instr.result);
            if (is_f32) {
                out() << "    movss " << dst << ", [rel " << label << "]\n";
            } else {
                out() << "    movsd " << dst << ", [rel " << label << "]\n";
            }
            break;
        }

        case IROpcode::FAdd:
        case IROpcode::FSub:
        case IROpcode::FMul:
        case IROpcode::FDiv: {
            std::string lhs = valXmmReg(instr.operands[0]);
            std::string rhs = valXmmReg(instr.operands[1]);
            std::string dst = allocXmmReg(instr.result);
            bool is_f32 = (instr.type == IRType::F32);
            const char* suffix = is_f32 ? "ss" : "sd";

            if (dst != lhs) {
                out() << "    movs" << suffix[1] << " " << dst << ", " << lhs << "\n";
            }

            const char* op_name;
            switch (instr.op) {
                case IROpcode::FAdd: op_name = "adds"; break;
                case IROpcode::FSub: op_name = "subs"; break;
                case IROpcode::FMul: op_name = "muls"; break;
                case IROpcode::FDiv: op_name = "divs"; break;
                default: op_name = "adds"; break;
            }
            out() << "    " << op_name << suffix[1] << " " << dst << ", " << rhs << "\n";
            break;
        }

        case IROpcode::FCmpEq:
        case IROpcode::FCmpNe:
        case IROpcode::FCmpLt:
        case IROpcode::FCmpLe:
        case IROpcode::FCmpGt:
        case IROpcode::FCmpGe: {
            std::string lhs = valXmmReg(instr.operands[0]);
            std::string rhs = valXmmReg(instr.operands[1]);
            IRType operand_type = getValueType(instr.operands[0]);
            bool is_f32 = (operand_type == IRType::F32);

            // Float comparison: use unsigned condition codes (above/below)
            const char* setcc;
            switch (instr.op) {
                case IROpcode::FCmpEq: setcc = "sete"; break;
                case IROpcode::FCmpNe: setcc = "setne"; break;
                case IROpcode::FCmpLt: setcc = "setb"; break;
                case IROpcode::FCmpLe: setcc = "setbe"; break;
                case IROpcode::FCmpGt: setcc = "seta"; break;
                case IROpcode::FCmpGe: setcc = "setae"; break;
                default: setcc = "sete"; break;
            }

            // Allocate dst and xor BEFORE ucomisd (xor clobbers FLAGS)
            std::string dst = allocReg(instr.result);
            std::string dst32 = regForWidth(dst, 32);
            std::string dst8 = regForWidth(dst, 8);
            out() << "    xor  " << dst32 << ", " << dst32 << "\n";

            if (is_f32) {
                out() << "    ucomiss " << lhs << ", " << rhs << "\n";
            } else {
                out() << "    ucomisd " << lhs << ", " << rhs << "\n";
            }
            out() << "    " << setcc << " " << dst8 << "\n";
            break;
        }

        case IROpcode::FNeg: {
            std::string src = valXmmReg(instr.operands[0]);
            std::string dst = allocXmmReg(instr.result);
            bool is_f32 = (instr.type == IRType::F32);

            // Negate by multiplying by -1.0
            std::string neg_label = addFloatConst(-1.0, is_f32);
            // Use a scratch xmm register for -1.0
            std::string tmp = allocXmmReg(INVALID_VALUE);
            if (is_f32) {
                out() << "    movss " << tmp << ", [rel " << neg_label << "]\n";
                if (dst != src) {
                    out() << "    movss " << dst << ", " << src << "\n";
                }
                out() << "    mulss " << dst << ", " << tmp << "\n";
            } else {
                out() << "    movsd " << tmp << ", [rel " << neg_label << "]\n";
                if (dst != src) {
                    out() << "    movsd " << dst << ", " << src << "\n";
                }
                out() << "    mulsd " << dst << ", " << tmp << "\n";
            }
            freeXmmReg(INVALID_VALUE);
            break;
        }

        case IROpcode::Call: {
            if (instr.is_tail_call) {
                // --- TAIL CALL (Phase A: arg setup + jmp to deferred) ---

                // Classify args into GPR and XMM
                int tc_gpr_count = 0;
                int tc_xmm_count = 0;
                struct TailArgInfo { size_t idx; bool is_float; std::string src; };
                std::vector<TailArgInfo> tc_gpr_args, tc_xmm_args;

                for (size_t i = 0; i < instr.operands.size(); ++i) {
                    IRType arg_type = getValueType(instr.operands[i]);
                    if (irTypeIsFloat(arg_type)) {
                        if (tc_xmm_count < MAX_XMM_ARG_REGS) {
                            std::string src = valXmmReg(instr.operands[i]);
                            tc_xmm_args.push_back({static_cast<size_t>(tc_xmm_count), true, src});
                            tc_xmm_count++;
                        }
                    } else {
                        if (tc_gpr_count < MAX_ARG_REGS) {
                            std::string src = valReg(instr.operands[i]);
                            tc_gpr_args.push_back({static_cast<size_t>(tc_gpr_count), false, src});
                            tc_gpr_count++;
                        }
                    }
                }

                // Move float args into xmm0..7
                for (auto& a : tc_xmm_args) {
                    if (a.src != XMM_ARG_REGS[a.idx]) {
                        IRType at = getValueType(instr.operands[a.idx]);
                        const char* sfx = (at == IRType::F32) ? "s" : "d";
                        out() << "    movs" << sfx << " " << XMM_ARG_REGS[a.idx] << ", " << a.src << "\n";
                    }
                }

                // GPR parallel move (same algorithm as normal call)
                size_t tc_n_gpr = tc_gpr_args.size();
                std::vector<bool> tc_done(tc_n_gpr, false);
                bool tc_progress = true;
                while (tc_progress) {
                    tc_progress = false;
                    for (size_t i = 0; i < tc_n_gpr; ++i) {
                        if (tc_done[i]) continue;
                        if (tc_gpr_args[i].src == ARG_REGS[tc_gpr_args[i].idx]) {
                            tc_done[i] = true; tc_progress = true; continue;
                        }
                        bool blocked = false;
                        for (size_t j = 0; j < tc_n_gpr; ++j) {
                            if (j != i && !tc_done[j] && tc_gpr_args[j].src == ARG_REGS[tc_gpr_args[i].idx]) {
                                blocked = true;
                                break;
                            }
                        }
                        if (!blocked) {
                            out() << "    mov  " << ARG_REGS[tc_gpr_args[i].idx] << ", " << tc_gpr_args[i].src << "\n";
                            tc_done[i] = true;
                            tc_progress = true;
                        }
                    }
                }
                for (size_t i = 0; i < tc_n_gpr; ++i) {
                    if (tc_done[i]) continue;
                    out() << "    mov  rax, " << tc_gpr_args[i].src << "\n";
                    size_t cur = i;
                    while (true) {
                        size_t next = tc_n_gpr;
                        for (size_t j = 0; j < tc_n_gpr; ++j) {
                            if (!tc_done[j] && j != cur && tc_gpr_args[j].src == ARG_REGS[tc_gpr_args[cur].idx]) {
                                next = j;
                                break;
                            }
                        }
                        if (next == tc_n_gpr) {
                            out() << "    mov  " << ARG_REGS[tc_gpr_args[cur].idx] << ", rax\n";
                            tc_done[cur] = true;
                            break;
                        }
                        out() << "    mov  " << ARG_REGS[tc_gpr_args[cur].idx] << ", " << tc_gpr_args[next].src << "\n";
                        tc_done[cur] = true;
                        cur = next;
                    }
                }

                // Jump to deferred epilogue block
                std::string label = "._tail_" + std::to_string(tail_call_counter_++);
                tail_call_sites_.push_back({label, instr.callee_name});
                out() << "    jmp  " << label << "\n";
                break;
            }

            // --- NORMAL CALL (existing code) ---
            // Save caller-saved GPRs
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

            // Ensure 16-byte RSP alignment before call:
            bool needs_align_pad = (to_save.size() % 2 != 0);
            if (needs_align_pad) {
                out() << "    sub  rsp, 8\n";
            }
            for (auto& [vid, reg] : to_save) {
                out() << "    push " << reg << "\n";
            }

            // Classify args into GPR and XMM
            int gpr_count = 0;
            int xmm_count = 0;
            struct ArgInfo { size_t idx; bool is_float; std::string src; };
            std::vector<ArgInfo> gpr_args, xmm_args;

            for (size_t i = 0; i < instr.operands.size(); ++i) {
                IRType arg_type = getValueType(instr.operands[i]);
                if (irTypeIsFloat(arg_type)) {
                    if (xmm_count < MAX_XMM_ARG_REGS) {
                        std::string src = valXmmReg(instr.operands[i]);
                        xmm_args.push_back({static_cast<size_t>(xmm_count), true, src});
                        xmm_count++;
                    }
                } else {
                    if (gpr_count < MAX_ARG_REGS) {
                        std::string src = valReg(instr.operands[i]);
                        gpr_args.push_back({static_cast<size_t>(gpr_count), false, src});
                        gpr_count++;
                    }
                }
            }

            // Move float args into xmm0..7
            for (auto& a : xmm_args) {
                if (a.src != XMM_ARG_REGS[a.idx]) {
                    // Determine suffix from operand type
                    IRType at = getValueType(instr.operands[a.idx]);
                    const char* sfx = (at == IRType::F32) ? "s" : "d";
                    out() << "    movs" << sfx << " " << XMM_ARG_REGS[a.idx] << ", " << a.src << "\n";
                }
            }

            // GPR parallel move (same algorithm as before)
            size_t n_gpr = gpr_args.size();
            std::vector<bool> done(n_gpr, false);
            bool progress = true;
            while (progress) {
                progress = false;
                for (size_t i = 0; i < n_gpr; ++i) {
                    if (done[i]) continue;
                    if (gpr_args[i].src == ARG_REGS[gpr_args[i].idx]) {
                        done[i] = true; progress = true; continue;
                    }
                    bool blocked = false;
                    for (size_t j = 0; j < n_gpr; ++j) {
                        if (j != i && !done[j] && gpr_args[j].src == ARG_REGS[gpr_args[i].idx]) {
                            blocked = true;
                            break;
                        }
                    }
                    if (!blocked) {
                        out() << "    mov  " << ARG_REGS[gpr_args[i].idx] << ", " << gpr_args[i].src << "\n";
                        done[i] = true;
                        progress = true;
                    }
                }
            }
            for (size_t i = 0; i < n_gpr; ++i) {
                if (done[i]) continue;
                out() << "    mov  rax, " << gpr_args[i].src << "\n";
                size_t cur = i;
                while (true) {
                    size_t next = n_gpr;
                    for (size_t j = 0; j < n_gpr; ++j) {
                        if (!done[j] && j != cur && gpr_args[j].src == ARG_REGS[gpr_args[cur].idx]) {
                            next = j;
                            break;
                        }
                    }
                    if (next == n_gpr) {
                        out() << "    mov  " << ARG_REGS[gpr_args[cur].idx] << ", rax\n";
                        done[cur] = true;
                        break;
                    }
                    out() << "    mov  " << ARG_REGS[gpr_args[cur].idx] << ", " << gpr_args[next].src << "\n";
                    done[cur] = true;
                    cur = next;
                }
            }

            out() << "    call _" << instr.callee_name << "\n";

            for (auto it = to_save.rbegin(); it != to_save.rend(); ++it) {
                out() << "    pop  " << it->second << "\n";
            }
            if (needs_align_pad) {
                out() << "    add  rsp, 8\n";
            }

            // Result: float return → xmm0, integer return → rax
            IRType result_type = getValueType(instr.result);
            if (irTypeIsFloat(result_type)) {
                std::string dst = allocXmmReg(instr.result);
                if (dst != "xmm0") {
                    std::string suffix = (result_type == IRType::F32) ? "ss" : "sd";
                    out() << "    movs" << suffix[1] << " " << dst << ", xmm0\n";
                }
            } else {
                std::string dst = allocReg(instr.result);
                if (dst != "rax") {
                    out() << "    mov  " << dst << ", rax\n";
                }
            }
            break;
        }
    }
}

} // namespace kern
