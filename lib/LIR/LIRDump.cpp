#include "kern/lir/LIRDump.h"
#include "kern/ir/Metadata.h"

namespace kern {

const char* lirOpName(LIROp op) {
    switch (op) {
        case LIROp::ConstInt:    return "const_int";
        case LIROp::ConstFloat:  return "const_float";
        case LIROp::ConstBool:   return "const_bool";
        case LIROp::ConstString: return "const_string";
        case LIROp::GlobalRef:   return "global_ref";
        case LIROp::Add:         return "add";
        case LIROp::Sub:         return "sub";
        case LIROp::Mul:         return "mul";
        case LIROp::Div:         return "div";
        case LIROp::Mod:         return "mod";
        case LIROp::FAdd:        return "fadd";
        case LIROp::FSub:        return "fsub";
        case LIROp::FMul:        return "fmul";
        case LIROp::FDiv:        return "fdiv";
        case LIROp::ICmpEq:      return "icmp_eq";
        case LIROp::ICmpNe:      return "icmp_ne";
        case LIROp::ICmpLt:      return "icmp_lt";
        case LIROp::ICmpLe:      return "icmp_le";
        case LIROp::ICmpGt:      return "icmp_gt";
        case LIROp::ICmpGe:      return "icmp_ge";
        case LIROp::FCmpEq:      return "fcmp_eq";
        case LIROp::FCmpNe:      return "fcmp_ne";
        case LIROp::FCmpLt:      return "fcmp_lt";
        case LIROp::FCmpLe:      return "fcmp_le";
        case LIROp::FCmpGt:      return "fcmp_gt";
        case LIROp::FCmpGe:      return "fcmp_ge";
        case LIROp::Neg:         return "neg";
        case LIROp::FNeg:        return "fneg";
        case LIROp::Not:         return "not";
        case LIROp::AddrOf:      return "addr_of";
        case LIROp::Load:        return "load";
        case LIROp::Store:       return "store";
        case LIROp::FieldPtr:    return "field_ptr";
        case LIROp::StructAlloc: return "struct_alloc";
        case LIROp::Branch:      return "br";
        case LIROp::CondBranch:  return "condbr";
        case LIROp::Ret:         return "ret";
        case LIROp::Call:        return "call";
        case LIROp::BlockArg:    return "block_arg";
    }
    return "?";
}

void dumpLIRInstr(const LIRInstr& i, const TypeTable& types, std::ostream& out) {
    if (i.result != INVALID_VREG) {
        out << "%v" << i.result << " = ";
    }

    out << lirOpName(i.op);

    switch (i.op) {
        case LIROp::ConstInt:
            out << " " << i.const_int.value;
            break;
        case LIROp::ConstFloat:
            out << " " << i.const_float.value;
            break;
        case LIROp::ConstBool:
            out << " " << (i.const_bool.value ? "true" : "false");
            break;
        case LIROp::ConstString:
            out << " @g" << i.const_string.global_index;
            break;
        case LIROp::GlobalRef:
            out << " @g" << i.global_ref.global_index;
            break;
        case LIROp::Add: case LIROp::Sub: case LIROp::Mul:
        case LIROp::Div: case LIROp::Mod:
        case LIROp::FAdd: case LIROp::FSub: case LIROp::FMul:
        case LIROp::FDiv:
        case LIROp::ICmpEq: case LIROp::ICmpNe: case LIROp::ICmpLt:
        case LIROp::ICmpLe: case LIROp::ICmpGt: case LIROp::ICmpGe:
        case LIROp::FCmpEq: case LIROp::FCmpNe: case LIROp::FCmpLt:
        case LIROp::FCmpLe: case LIROp::FCmpGt: case LIROp::FCmpGe:
            out << " %v" << i.bin.lhs << ", %v" << i.bin.rhs;
            break;
        case LIROp::Neg: case LIROp::FNeg: case LIROp::Not:
            out << " %v" << i.unary.operand;
            break;
        case LIROp::AddrOf:
            out << " %v" << i.addr_of.source;
            break;
        case LIROp::Load:
            out << " %v" << i.load.ptr;
            break;
        case LIROp::Store:
            out << " %v" << i.store.ptr << ", %v" << i.store.value;
            break;
        case LIROp::FieldPtr:
            out << " %v" << i.field_ptr.base << ", +" << i.field_ptr.offset;
            break;
        case LIROp::StructAlloc:
            out << " size=" << i.struct_alloc.size << " align=" << i.struct_alloc.align;
            break;
        case LIROp::Branch:
            out << " bb" << i.branch.target;
            break;
        case LIROp::CondBranch:
            out << " %v" << i.cond_branch.cond
                << ", bb" << i.cond_branch.true_target
                << ", bb" << i.cond_branch.false_target;
            break;
        case LIROp::Ret:
            if (i.ret.value != INVALID_VREG)
                out << " %v" << i.ret.value;
            break;
        case LIROp::Call:
            if (i.call.is_tail) out << " [tail]";
            out << " @" << i.call.callee;
            out << "(";
            for (uint32_t a = 0; a < i.call.arg_count; ++a) {
                if (a > 0) out << ", ";
                out << "%v" << i.call.args[a];
            }
            out << ")";
            break;
        case LIROp::BlockArg:
            out << " $" << i.block_arg.index;
            break;
    }

    out << " : " << types.name(i.type);
}

void dumpLIRFunction(const LIRFunction* fn, const TypeTable& types, std::ostream& out) {
    out << "fn @" << fn->name;
    out << " [" << purityName(static_cast<Purity>(fn->purity)) << "]";
    if (fn->is_recursive) {
        out << (fn->is_tail_recursive ? " [tail-recursive]" : " [recursive]");
    }
    if (fn->is_intrinsic) out << " [intrinsic]";
    out << "\n";

    // Parameters
    out << "  params: (";
    for (uint32_t p = 0; p < fn->param_count; ++p) {
        if (p > 0) out << ", ";
        out << types.name(fn->param_types[p]);
    }
    out << ") -> " << types.name(fn->return_type) << "\n";

    // Blocks
    for (uint32_t b = 0; b < fn->block_count; ++b) {
        auto& block = fn->blocks[b];
        out << "  " << block.label;
        if (block.param_count > 0) {
            out << "(";
            for (uint32_t p = 0; p < block.param_count; ++p) {
                if (p > 0) out << ", ";
                out << types.name(block.param_types[p]);
            }
            out << ")";
        }
        out << ":\n";
        for (uint32_t i = 0; i < block.instr_count; ++i) {
            out << "    ";
            dumpLIRInstr(block.instrs[i], types, out);
            out << "\n";
        }
    }
}

void dumpLIR(const LIRModule* mod, const TypeTable& types, std::ostream& out) {
    // Globals
    if (mod->global_count > 0) {
        out << "; globals\n";
        for (uint32_t i = 0; i < mod->global_count; ++i) {
            auto& g = mod->globals[i];
            out << "@g" << g.index << " = ";
            if (g.kind == GlobalData::StringLit) {
                out << "string \"";
                for (uint32_t c = 0; c < g.string_lit.length; ++c) {
                    char ch = g.string_lit.data[c];
                    if (ch == '\n') out << "\\n";
                    else if (ch == '\t') out << "\\t";
                    else if (ch == '\\') out << "\\\\";
                    else if (ch == '"') out << "\\\"";
                    else out << ch;
                }
                out << "\" (" << g.string_lit.length << " bytes)";
            } else {
                out << "float " << g.float_const.value
                    << (g.float_const.is_f32 ? " (f32)" : " (f64)");
            }
            out << "\n";
        }
        out << "\n";
    }

    // Functions
    for (uint32_t i = 0; i < mod->fn_count; ++i) {
        dumpLIRFunction(&mod->functions[i], types, out);
        out << "\n";
    }
}

} // namespace kern
