#include "kern/lir/LIRDump.h"
#include "kern/hir/HIR.h"

namespace kern {

const char* lirOpName(LIROp op) {
    switch (op) {
        case LIROp::ConstInt:    return "const_int";
        case LIROp::ConstFloat:  return "const_float";
        case LIROp::ConstBool:   return "const_bool";
        case LIROp::ConstString: return "const_string";
        case LIROp::ConstCString: return "const_cstring";
        case LIROp::GlobalRef:   return "global_ref";
        case LIROp::Add:         return "add";
        case LIROp::Sub:         return "sub";
        case LIROp::Mul:         return "mul";
        case LIROp::Div:         return "div";
        case LIROp::Mod:         return "mod";
        case LIROp::AddWrap:     return "add_wrap";
        case LIROp::SubWrap:     return "sub_wrap";
        case LIROp::MulWrap:     return "mul_wrap";
        case LIROp::AddSat:      return "add_sat";
        case LIROp::SubSat:      return "sub_sat";
        case LIROp::BAnd:        return "band";
        case LIROp::BOr:         return "bor";
        case LIROp::BXor:        return "bxor";
        case LIROp::Shl:         return "shl";
        case LIROp::Shr:         return "shr";
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
        case LIROp::BNot:        return "bnot";
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
        case LIROp::Cast:        return "cast";
        case LIROp::InlineAsm:      return "inline_asm";
        case LIROp::CallIndirect:   return "call_indirect";
        case LIROp::FnRef:          return "fn_ref";
        case LIROp::AtomicLoad:     return "atomic_load";
        case LIROp::AtomicStore:    return "atomic_store";
        case LIROp::AtomicCas:      return "atomic_cas";
        case LIROp::AtomicFetchAdd: return "atomic_fetch_add";
        case LIROp::AtomicFetchSub: return "atomic_fetch_sub";
        case LIROp::AtomicFetchAnd: return "atomic_fetch_and";
        case LIROp::AtomicFetchOr:  return "atomic_fetch_or";
        case LIROp::AtomicFetchXor: return "atomic_fetch_xor";
        case LIROp::AtomicCas128:  return "atomic_cas128";
        case LIROp::Fence:          return "fence";
        case LIROp::CompilerFence:  return "compiler_fence";
        case LIROp::PercpuLoad:     return "percpu_load";
        case LIROp::PercpuStore:    return "percpu_store";
        case LIROp::LoadGlobal:     return "load_global";
        case LIROp::StoreGlobal:    return "store_global";
        case LIROp::Clz:            return "clz";
        case LIROp::Ctz:            return "ctz";
        case LIROp::Popcnt:         return "popcnt";
        case LIROp::Bswap:          return "bswap";
        case LIROp::PortIn:         return "port_in";
        case LIROp::PortOut:        return "port_out";
        case LIROp::Trap:           return "trap";
        case LIROp::Switch:         return "switch";
        case LIROp::VaStart:        return "va_start";
        case LIROp::VaArg:          return "va_arg";
        case LIROp::Alloca:         return "alloca";
        case LIROp::TlsLoad:        return "tls_load";
        case LIROp::TlsStore:       return "tls_store";
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
        case LIROp::ConstCString:
            out << " @g" << i.const_string.global_index;
            break;
        case LIROp::GlobalRef:
            out << " @g" << i.global_ref.global_index;
            break;
        case LIROp::Add: case LIROp::Sub: case LIROp::Mul:
        case LIROp::Div: case LIROp::Mod:
        case LIROp::AddWrap: case LIROp::SubWrap: case LIROp::MulWrap:
        case LIROp::AddSat: case LIROp::SubSat:
        case LIROp::BAnd: case LIROp::BOr: case LIROp::BXor:
        case LIROp::Shl: case LIROp::Shr:
        case LIROp::FAdd: case LIROp::FSub: case LIROp::FMul:
        case LIROp::FDiv:
        case LIROp::ICmpEq: case LIROp::ICmpNe: case LIROp::ICmpLt:
        case LIROp::ICmpLe: case LIROp::ICmpGt: case LIROp::ICmpGe:
        case LIROp::FCmpEq: case LIROp::FCmpNe: case LIROp::FCmpLt:
        case LIROp::FCmpLe: case LIROp::FCmpGt: case LIROp::FCmpGe:
            out << " %v" << i.bin.lhs << ", %v" << i.bin.rhs;
            break;
        case LIROp::Neg: case LIROp::FNeg: case LIROp::Not: case LIROp::BNot:
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
            if (i.branch.arg_count > 0) {
                out << "(";
                for (uint32_t a = 0; a < i.branch.arg_count; ++a) {
                    if (a > 0) out << ", ";
                    out << "%v" << i.branch.args[a];
                }
                out << ")";
            }
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
        case LIROp::Cast:
            out << " %v" << i.cast.operand
                << " : " << types.name(i.cast.src_type)
                << " -> " << types.name(i.type);
            break;
        case LIROp::InlineAsm:
            out << " " << i.inline_asm.line_count << " lines";
            for (uint32_t l = 0; l < i.inline_asm.line_count; ++l) {
                out << "\n      ; " << std::string_view(i.inline_asm.lines[l],
                                                         i.inline_asm.line_lengths[l]);
            }
            break;
        case LIROp::CallIndirect:
            out << " %v" << i.call_indirect.callee;
            out << "(";
            for (uint32_t a = 0; a < i.call_indirect.arg_count; ++a) {
                if (a > 0) out << ", ";
                out << "%v" << i.call_indirect.args[a];
            }
            out << ")";
            break;
        case LIROp::FnRef:
            out << " @" << i.fn_ref.fn_name;
            break;
        case LIROp::AtomicLoad:
            out << " %v" << i.atomic_load.ptr << " order=" << static_cast<int>(i.atomic_load.order);
            break;
        case LIROp::AtomicStore:
            out << " %v" << i.atomic_store.ptr << ", %v" << i.atomic_store.value
                << " order=" << static_cast<int>(i.atomic_store.order);
            break;
        case LIROp::AtomicCas:
            out << " %v" << i.atomic_cas.ptr << ", %v" << i.atomic_cas.expected
                << ", %v" << i.atomic_cas.desired
                << " order=" << static_cast<int>(i.atomic_cas.order);
            break;
        case LIROp::AtomicCas128:
            out << " %v" << i.atomic_cas128.ptr
                << ", %v" << i.atomic_cas128.exp_lo << ":%v" << i.atomic_cas128.exp_hi
                << ", %v" << i.atomic_cas128.des_lo << ":%v" << i.atomic_cas128.des_hi
                << " order=" << static_cast<int>(i.atomic_cas128.order);
            break;
        case LIROp::AtomicFetchAdd:
            out << " %v" << i.atomic_fetch_add.ptr << ", %v" << i.atomic_fetch_add.value
                << " order=" << static_cast<int>(i.atomic_fetch_add.order);
            break;
        case LIROp::AtomicFetchSub:
            out << " %v" << i.atomic_fetch_sub.ptr << ", %v" << i.atomic_fetch_sub.value
                << " order=" << static_cast<int>(i.atomic_fetch_sub.order);
            break;
        case LIROp::AtomicFetchAnd:
        case LIROp::AtomicFetchOr:
        case LIROp::AtomicFetchXor:
            out << " %v" << i.atomic_rmw.ptr << ", %v" << i.atomic_rmw.value
                << " order=" << static_cast<int>(i.atomic_rmw.order);
            break;
        case LIROp::Fence:
            out << " order=" << static_cast<int>(i.fence.order);
            break;
        case LIROp::CompilerFence:
            break;
        case LIROp::PercpuLoad:
            out << " %v" << i.percpu_load.offset;
            break;
        case LIROp::PercpuStore:
            out << " %v" << i.percpu_store.offset << ", %v" << i.percpu_store.value;
            break;
        case LIROp::LoadGlobal:
            out << " @" << i.load_global.label;
            break;
        case LIROp::StoreGlobal:
            out << " @" << i.store_global.label << ", %v" << i.store_global.value;
            break;
        case LIROp::Clz: case LIROp::Ctz: case LIROp::Popcnt: case LIROp::Bswap:
            out << " %v" << i.unary.operand;
            break;
        case LIROp::PortIn:
            out << " %v" << i.port_in.port;
            break;
        case LIROp::PortOut:
            out << " %v" << i.port_out.port << ", %v" << i.port_out.value;
            break;
        case LIROp::Trap:
            break; // no operands
        case LIROp::Switch:
            out << " %v" << i.switch_.scrutinee << " [";
            for (uint32_t c = 0; c < i.switch_.case_count; ++c) {
                if (c > 0) out << ", ";
                out << i.switch_.cases[c].value << "->bb" << i.switch_.cases[c].target_block;
            }
            out << "] default->bb" << i.switch_.default_block;
            break;
        case LIROp::VaStart:
            out << " fixed=" << i.va_start.fixed_param_count;
            break;
        case LIROp::VaArg:
            out << " %v" << i.va_arg.ap;
            break;
        case LIROp::Alloca:
            out << " %v" << i.alloca_.size;
            break;
        case LIROp::TlsLoad:
            out << " %v" << i.tls_load.offset;
            break;
        case LIROp::TlsStore:
            out << " %v" << i.tls_store.offset << ", %v" << i.tls_store.value;
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
            } else if (g.kind == GlobalData::Variable) {
                out << (g.variable.is_mutable ? "var " : "val ")
                    << g.label << " = " << g.variable.init_value
                    << " (" << static_cast<int>(g.variable.size) << "B)";
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
