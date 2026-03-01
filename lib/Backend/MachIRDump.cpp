#include "kern/backend/MachIRDump.h"
#include "kern/lir/LIR.h"


namespace kern {

static void dumpOperand(const MachOperand& op, uint8_t width, std::ostream& out) {
    switch (op.kind) {
        case MachOperand::Reg:
            if (op.is_physical) {
                out << physRegName(op.phys, width);
            } else {
                out << "%v" << op.vreg;
            }
            break;
        case MachOperand::Imm:
            out << op.imm;
            break;
        case MachOperand::Stack:
            out << "[rbp" << (op.stack_offset >= 0 ? "+" : "") << op.stack_offset << "]";
            break;
        case MachOperand::Label:
            out << op.label;
            break;
        case MachOperand::None:
            out << "<none>";
            break;
    }
}

void dumpMachInstr(const MachInstr& instr, std::ostream& out) {
    out << "    ";

    // Handle conditional suffix for setcc/jcc
    if (instr.op == X86Op::Setcc) {
        out << "set" << condCodeSuffix(instr.cc);
    } else if (instr.op == X86Op::Jcc) {
        out << "j" << condCodeSuffix(instr.cc);
    } else if (instr.op == X86Op::Pseudo_ParallelMove) {
        out << "parallel_move";
        for (uint8_t i = 0; i + 1 < instr.operand_count; i += 2) {
            if (i > 0) out << ",";
            out << " ";
            dumpOperand(instr.operand(i), 64, out);
            out << " <- ";
            dumpOperand(instr.operand(i + 1), 64, out);
        }
        out << "\n";
        return;
    } else if (instr.op == X86Op::Pseudo_FrameSetup) {
        out << "frame_setup\n";
        return;
    } else if (instr.op == X86Op::Pseudo_FrameDestroy) {
        out << "frame_destroy\n";
        return;
    } else if (instr.op == X86Op::InlineAsm) {
        out << "inline_asm " << instr.asm_data.line_count << " lines";
        for (uint32_t i = 0; i < instr.asm_data.line_count; ++i) {
            out << "\n      ; " << std::string_view(instr.asm_data.lines[i],
                                                     instr.asm_data.line_lengths[i]);
        }
        out << "\n";
        return;
    } else {
        out << x86OpName(instr.op);
    }

    // Operands
    for (uint8_t i = 0; i < instr.operand_count; ++i) {
        if (i == 0) out << " ";
        else out << ", ";
        dumpOperand(instr.operand(i), instr.width, out);
    }
    out << "\n";
}

void dumpMachFunction(const MachFunction& fn, const TypeTable& /* types */,
                      std::ostream& out) {
    out << "fn @" << fn.name;
    if (fn.is_intrinsic) {
        out << " = intrinsic\n\n";
        return;
    }
    out << " (stack=" << fn.stack_size << "):\n";

    for (uint32_t b = 0; b < fn.block_count; ++b) {
        const auto& block = fn.blocks[b];
        out << block.label << ":\n";
        for (uint32_t i = 0; i < block.instr_count; ++i) {
            dumpMachInstr(block.instrs[i], out);
        }
    }
    out << "\n";
}

void dumpMachIR(const MachModule* mod, const LIRModule* lir_mod,
                const TypeTable& types, std::ostream& out) {
    // Globals from LIR module
    if (lir_mod && lir_mod->global_count > 0) {
        out << "; globals:\n";
        for (uint32_t i = 0; i < lir_mod->global_count; ++i) {
            const auto& g = lir_mod->globals[i];
            out << ";   " << g.label << " = ";
            if (g.kind == GlobalData::StringLit) {
                out << "\"";
                for (uint32_t j = 0; j < g.string_lit.length; ++j) {
                    char c = g.string_lit.data[j];
                    if (c == '\n') out << "\\n";
                    else if (c == '\t') out << "\\t";
                    else if (c == '\\') out << "\\\\";
                    else if (c == '"') out << "\\\"";
                    else out << c;
                }
                out << "\"\n";
            } else {
                out << g.float_const.value;
                if (g.float_const.is_f32) out << "f";
                out << "\n";
            }
        }
        out << "\n";
    }

    for (uint32_t i = 0; i < mod->fn_count; ++i) {
        dumpMachFunction(mod->functions[i], types, out);
    }
}

} // namespace kern
