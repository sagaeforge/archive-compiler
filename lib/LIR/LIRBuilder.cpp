#include "kern/lir/LIRBuilder.h"
#include <cassert>
#include <cstring>
#include <sstream>

namespace kern {

LIRBuilder::LIRBuilder(CompilationContext& ctx) : ctx_(ctx) {}

static bool blockTerminated(const std::vector<LIRInstr>& instrs) {
    if (instrs.empty()) return false;
    auto op = instrs.back().op;
    return op == LIROp::Ret || op == LIROp::Branch || op == LIROp::CondBranch;
}

VReg LIRBuilder::freshVReg() {
    return next_vreg_++;
}

void LIRBuilder::emit(LIRInstr instr) {
    blocks_[current_block_].instrs.push_back(instr);
}

uint32_t LIRBuilder::newBlock(std::string_view label) {
    uint32_t idx = static_cast<uint32_t>(blocks_.size());
    // Make labels unique and NASM-local (dot prefix) by appending counter
    char buf[128];
    int len = snprintf(buf, sizeof(buf), ".%.*s_%u",
                       static_cast<int>(label.size()), label.data(),
                       label_counter_++);
    auto interned = ctx_.strings.intern(std::string_view(buf, len));
    blocks_.push_back(BlockBuild{interned, {}, {}});
    return idx;
}

void LIRBuilder::switchToBlock(uint32_t block_idx) {
    current_block_ = block_idx;
}

void LIRBuilder::emitBranch(uint32_t target) {
    LIRInstr i{};
    i.op = LIROp::Branch;
    i.result = INVALID_VREG;
    i.type = TypeTable::Unit;
    i.branch.target = target;
    i.branch.args = nullptr;
    i.branch.arg_count = 0;
    emit(i);
}

void LIRBuilder::emitBranchWithArgs(uint32_t target, const std::vector<VReg>& args) {
    LIRInstr i{};
    i.op = LIROp::Branch;
    i.result = INVALID_VREG;
    i.type = TypeTable::Unit;
    i.branch.target = target;
    i.branch.arg_count = static_cast<uint32_t>(args.size());
    if (!args.empty()) {
        i.branch.args = ctx_.arena.makeArray<VReg>(args.size());
        for (size_t j = 0; j < args.size(); ++j) {
            i.branch.args[j] = args[j];
        }
    } else {
        i.branch.args = nullptr;
    }
    emit(i);
}

void LIRBuilder::emitCondBranch(VReg cond, uint32_t true_bb, uint32_t false_bb) {
    LIRInstr i{};
    i.op = LIROp::CondBranch;
    i.result = INVALID_VREG;
    i.type = TypeTable::Unit;
    i.cond_branch.cond = cond;
    i.cond_branch.true_target = true_bb;
    i.cond_branch.false_target = false_bb;
    emit(i);
}

uint32_t LIRBuilder::addStringGlobal(const char* data, uint32_t length) {
    uint32_t idx = static_cast<uint32_t>(globals_.size());
    GlobalData g{};
    g.kind = GlobalData::StringLit;
    g.index = idx;
    std::ostringstream label_ss;
    label_ss << "_str_" << idx;
    auto label_str = label_ss.str();
    g.label = ctx_.strings.intern(std::string_view(label_str));
    char* arena_data = static_cast<char*>(ctx_.arena.allocate(length, 1));
    std::memcpy(arena_data, data, length);
    g.string_lit.data = arena_data;
    g.string_lit.length = length;
    globals_.push_back(g);
    return idx;
}

uint32_t LIRBuilder::addFloatGlobal(double value, bool is_f32) {
    for (uint32_t i = 0; i < globals_.size(); ++i) {
        auto& g = globals_[i];
        if (g.kind == GlobalData::FloatConst &&
            g.float_const.value == value &&
            g.float_const.is_f32 == is_f32) {
            return i;
        }
    }
    uint32_t idx = static_cast<uint32_t>(globals_.size());
    GlobalData g{};
    g.kind = GlobalData::FloatConst;
    g.index = idx;
    std::ostringstream label_ss;
    label_ss << "_float_" << idx;
    auto label_str = label_ss.str();
    g.label = ctx_.strings.intern(std::string_view(label_str));
    g.float_const.value = value;
    g.float_const.is_f32 = is_f32;
    globals_.push_back(g);
    return idx;
}

uint32_t LIRBuilder::structFieldOffset(TypeId struct_type, std::string_view field_name) {
    auto& ti = ctx_.types.get(struct_type);
    if (ti.kind == TypeKind::Struct) {
        for (uint32_t i = 0; i < ti.struct_.field_count; ++i) {
            if (ti.struct_.fields[i].name == field_name) {
                if (ti.struct_.fields[i].offset >= 0)
                    return static_cast<uint32_t>(ti.struct_.fields[i].offset);
                return i * 8;
            }
        }
    }
    if (ti.kind == TypeKind::Union) {
        return 8;
    }
    return 0;
}

uint32_t LIRBuilder::structSize(TypeId struct_type) {
    return ctx_.types.sizeOf(struct_type);
}

uint32_t LIRBuilder::structAlign(TypeId struct_type) {
    return ctx_.types.alignOf(struct_type);
}

int64_t LIRBuilder::enumVariantValue(TypeId enum_type, std::string_view variant_name) {
    auto& ti = ctx_.types.get(enum_type);
    assert(ti.kind == TypeKind::Enum);
    for (uint32_t i = 0; i < ti.enum_.variant_count; ++i) {
        if (ti.enum_.names[i] == variant_name) return ti.enum_.values[i];
    }
    return 0;
}

uint32_t LIRBuilder::unionVariantTag(TypeId union_type, std::string_view variant_name) {
    auto& ti = ctx_.types.get(union_type);
    assert(ti.kind == TypeKind::Union);
    for (uint32_t i = 0; i < ti.union_.variant_count; ++i) {
        if (ti.union_.variants[i].name == variant_name) return i;
    }
    assert(false && "union variant not found — should be caught by HIRBuilder");
    return 0;
}

// ============================================================================
// Module / Function building
// ============================================================================

LIRModule* LIRBuilder::build(const HIRModule* hir) {
    auto* mod = ctx_.arena.make<LIRModule>();
    mod->fn_count = hir->fn_count;
    mod->functions = ctx_.arena.makeArray<LIRFunction>(hir->fn_count);

    for (uint32_t i = 0; i < hir->fn_count; ++i) {
        mod->functions[i] = buildFunction(hir->functions[i]);
    }

    mod->global_count = static_cast<uint32_t>(globals_.size());
    if (mod->global_count > 0) {
        mod->globals = ctx_.arena.makeArray<GlobalData>(mod->global_count);
        for (uint32_t i = 0; i < mod->global_count; ++i) {
            mod->globals[i] = globals_[i];
        }
    } else {
        mod->globals = nullptr;
    }

    return mod;
}

LIRFunction LIRBuilder::buildFunction(const HIRFnDecl* fn) {
    next_vreg_ = 0;
    blocks_.clear();
    locals_.clear();
    var_addrs_.clear();
    label_counter_ = 0;

    LIRFunction lir_fn{};
    lir_fn.name = fn->name;
    lir_fn.param_count = fn->param_count;
    lir_fn.return_type = fn->return_type;
    lir_fn.purity = fn->purity;
    lir_fn.is_recursive = fn->is_recursive;
    lir_fn.is_tail_recursive = fn->is_tail_recursive;
    lir_fn.is_intrinsic = fn->is_intrinsic;
    lir_fn.is_naked = fn->is_naked;
    lir_fn.is_interrupt = fn->is_interrupt;
    lir_fn.section_name = fn->section_name;

    if (fn->param_count > 0) {
        lir_fn.param_types = ctx_.arena.makeArray<TypeId>(fn->param_count);
        for (uint32_t i = 0; i < fn->param_count; ++i) {
            lir_fn.param_types[i] = fn->params[i].type;
        }
    } else {
        lir_fn.param_types = nullptr;
    }

    if (fn->is_intrinsic || !fn->body) {
        lir_fn.blocks = nullptr;
        lir_fn.block_count = 0;
        lir_fn.next_vreg = 0;
        return lir_fn;
    }

    uint32_t entry = newBlock("entry");
    switchToBlock(entry);

    for (uint32_t i = 0; i < fn->param_count; ++i) {
        VReg vreg = freshVReg();
        LIRInstr arg_instr{};
        arg_instr.op = LIROp::BlockArg;
        arg_instr.result = vreg;
        arg_instr.type = fn->params[i].type;
        arg_instr.block_arg.index = i;
        emit(arg_instr);
        locals_[fn->params[i].name] = vreg;
    }

    for (uint32_t i = 0; i < fn->param_count; ++i) {
        blocks_[entry].param_types.push_back(fn->params[i].type);
    }

    VReg result = lowerExpr(fn->body);

    // Emit return if block not already terminated
    auto& cur = blocks_[current_block_].instrs;
    if (cur.empty() || cur.back().op != LIROp::Ret) {
        LIRInstr ret{};
        ret.op = LIROp::Ret;
        ret.result = INVALID_VREG;
        ret.type = fn->return_type;
        ret.ret.value = (fn->return_type == TypeTable::Unit) ? INVALID_VREG : result;
        emit(ret);
    }

    lir_fn.next_vreg = next_vreg_;
    finalizeBlocks(lir_fn);
    return lir_fn;
}

void LIRBuilder::finalizeBlocks(LIRFunction& fn) {
    fn.block_count = static_cast<uint32_t>(blocks_.size());
    fn.blocks = ctx_.arena.makeArray<LIRBlock>(fn.block_count);

    for (uint32_t b = 0; b < fn.block_count; ++b) {
        auto& src = blocks_[b];
        auto& dst = fn.blocks[b];
        dst.label = src.label;

        dst.param_count = static_cast<uint32_t>(src.param_types.size());
        if (dst.param_count > 0) {
            dst.param_types = ctx_.arena.makeArray<TypeId>(dst.param_count);
            for (uint32_t p = 0; p < dst.param_count; ++p)
                dst.param_types[p] = src.param_types[p];
        } else {
            dst.param_types = nullptr;
        }

        dst.instr_count = static_cast<uint32_t>(src.instrs.size());
        if (dst.instr_count > 0) {
            dst.instrs = ctx_.arena.makeArray<LIRInstr>(dst.instr_count);
            for (uint32_t i = 0; i < dst.instr_count; ++i)
                dst.instrs[i] = src.instrs[i];
        } else {
            dst.instrs = nullptr;
        }
    }
}

// ============================================================================
// Expression lowering
// ============================================================================

VReg LIRBuilder::lowerExpr(const HIRExpr* expr) {
    switch (expr->kind) {
        case HIRExpr::Kind::IntLit:
            return lowerIntLit(static_cast<const HIRIntLitExpr*>(expr));
        case HIRExpr::Kind::FloatLit:
            return lowerFloatLit(static_cast<const HIRFloatLitExpr*>(expr));
        case HIRExpr::Kind::BoolLit:
            return lowerBoolLit(static_cast<const HIRBoolLitExpr*>(expr));
        case HIRExpr::Kind::StringLit:
            return lowerStringLit(static_cast<const HIRStringLitExpr*>(expr));
        case HIRExpr::Kind::Ident:
            return lowerIdent(static_cast<const HIRIdentExpr*>(expr));
        case HIRExpr::Kind::BinOp:
            return lowerBinOp(static_cast<const HIRBinOpExpr*>(expr));
        case HIRExpr::Kind::UnaryOp:
            return lowerUnaryOp(static_cast<const HIRUnaryOpExpr*>(expr));
        case HIRExpr::Kind::Call:
            return lowerCall(static_cast<const HIRCallExpr*>(expr));
        case HIRExpr::Kind::If:
            return lowerIf(static_cast<const HIRIfExpr*>(expr));
        case HIRExpr::Kind::Match:
            return lowerMatch(static_cast<const HIRMatchExpr*>(expr));
        case HIRExpr::Kind::Block:
            return lowerBlock(static_cast<const HIRBlockExpr*>(expr));
        case HIRExpr::Kind::Return:
            return lowerReturn(static_cast<const HIRReturnExpr*>(expr));
        case HIRExpr::Kind::StructLit:
            return lowerStructLit(static_cast<const HIRStructLitExpr*>(expr));
        case HIRExpr::Kind::FieldAccess:
            return lowerFieldAccess(static_cast<const HIRFieldAccessExpr*>(expr));
        case HIRExpr::Kind::EnumAccess:
            return lowerEnumAccess(static_cast<const HIREnumAccessExpr*>(expr));
        case HIRExpr::Kind::UnionVariant:
            return lowerUnionVariant(static_cast<const HIRUnionVariantExpr*>(expr));
        case HIRExpr::Kind::AddrOf:
            return lowerAddrOf(static_cast<const HIRAddrOfExpr*>(expr));
        case HIRExpr::Kind::Deref:
            return lowerDeref(static_cast<const HIRDerefExpr*>(expr));
        case HIRExpr::Kind::Cast:
            return lowerCast(static_cast<const HIRCastExpr*>(expr));
        case HIRExpr::Kind::Loop:
            return lowerLoop(static_cast<const HIRLoopExpr*>(expr));
        case HIRExpr::Kind::Break:
            return lowerBreak(static_cast<const HIRBreakExpr*>(expr));
        case HIRExpr::Kind::Continue:
            return lowerContinue(static_cast<const HIRContinueExpr*>(expr));
        case HIRExpr::Kind::ArrayLit:
            return lowerArrayLit(static_cast<const HIRArrayLitExpr*>(expr));
        case HIRExpr::Kind::IndexAccess:
            return lowerIndexAccess(static_cast<const HIRIndexAccessExpr*>(expr));
        case HIRExpr::Kind::InlineAsm:
            return lowerInlineAsm(static_cast<const HIRInlineAsmExpr*>(expr));
        case HIRExpr::Kind::FnRef:
            return lowerFnRef(static_cast<const HIRFnRefExpr*>(expr));
        case HIRExpr::Kind::CallIndirect:
            return lowerCallIndirect(static_cast<const HIRCallIndirectExpr*>(expr));
    }
    return INVALID_VREG;
}

VReg LIRBuilder::lowerIntLit(const HIRIntLitExpr* expr) {
    VReg r = freshVReg();
    LIRInstr i{};
    i.op = LIROp::ConstInt;
    i.result = r;
    i.type = expr->type;
    i.const_int.value = expr->value;
    i.loc = expr->loc;
    emit(i);
    return r;
}

VReg LIRBuilder::lowerFloatLit(const HIRFloatLitExpr* expr) {
    bool is_f32 = (expr->type == TypeTable::F32);
    uint32_t gi = addFloatGlobal(expr->value, is_f32);

    VReg r = freshVReg();
    LIRInstr i{};
    i.op = LIROp::GlobalRef;
    i.result = r;
    i.type = expr->type;
    i.global_ref.global_index = gi;
    i.loc = expr->loc;
    emit(i);
    return r;
}

VReg LIRBuilder::lowerBoolLit(const HIRBoolLitExpr* expr) {
    VReg r = freshVReg();
    LIRInstr i{};
    i.op = LIROp::ConstBool;
    i.result = r;
    i.type = TypeTable::Bool;
    i.const_bool.value = expr->value;
    i.loc = expr->loc;
    emit(i);
    return r;
}

VReg LIRBuilder::lowerStringLit(const HIRStringLitExpr* expr) {
    uint32_t gi = addStringGlobal(expr->data, expr->length);

    VReg r = freshVReg();
    LIRInstr i{};
    i.op = LIROp::ConstString;
    i.result = r;
    i.type = expr->type;
    i.const_string.global_index = gi;
    i.loc = expr->loc;
    emit(i);
    return r;
}

VReg LIRBuilder::lowerIdent(const HIRIdentExpr* expr) {
    auto var_it = var_addrs_.find(expr->name);
    if (var_it != var_addrs_.end()) {
        VReg r = freshVReg();
        LIRInstr i{};
        i.op = LIROp::Load;
        i.result = r;
        i.type = expr->type;
        i.load.ptr = var_it->second;
        i.loc = expr->loc;
        emit(i);
        return r;
    }

    auto it = locals_.find(expr->name);
    if (it != locals_.end()) return it->second;

    return INVALID_VREG;
}

VReg LIRBuilder::lowerBinOp(const HIRBinOpExpr* expr) {
    // Short-circuit And/Or
    if (expr->op == HIRBinOp::And) {
        VReg lhs = lowerExpr(expr->lhs);
        return lowerAndOr(lhs, expr->rhs, true, expr->loc);
    }
    if (expr->op == HIRBinOp::Or) {
        VReg lhs = lowerExpr(expr->lhs);
        return lowerAndOr(lhs, expr->rhs, false, expr->loc);
    }

    VReg lhs = lowerExpr(expr->lhs);
    VReg rhs = lowerExpr(expr->rhs);

    // Pointer arithmetic: scale integer operand by sizeof(pointee)
    auto lhs_kind = (expr->lhs->type < ctx_.types.size())
        ? ctx_.types.get(expr->lhs->type).kind : TypeKind::Primitive;
    auto rhs_kind = (expr->rhs->type < ctx_.types.size())
        ? ctx_.types.get(expr->rhs->type).kind : TypeKind::Primitive;
    bool lhs_is_ptr = (lhs_kind == TypeKind::Ptr || lhs_kind == TypeKind::PtrMut);
    bool rhs_is_ptr = (rhs_kind == TypeKind::Ptr || rhs_kind == TypeKind::PtrMut);

    if ((expr->op == HIRBinOp::Add || expr->op == HIRBinOp::Sub) &&
        (lhs_is_ptr || rhs_is_ptr)) {
        // Get pointee size
        TypeId ptr_type = lhs_is_ptr ? expr->lhs->type : expr->rhs->type;
        TypeId pointee = ctx_.types.get(ptr_type).ptr.pointee;
        uint32_t elem_size = ctx_.types.sizeOf(pointee);

        VReg int_operand = lhs_is_ptr ? rhs : lhs;
        VReg ptr_operand = lhs_is_ptr ? lhs : rhs;

        // Scale: int_operand * elem_size
        if (elem_size > 1) {
            VReg scale = freshVReg();
            LIRInstr sc{};
            sc.op = LIROp::ConstInt;
            sc.result = scale;
            sc.type = TypeTable::I64;
            sc.const_int.value = static_cast<int64_t>(elem_size);
            sc.loc = expr->loc;
            emit(sc);

            VReg scaled = freshVReg();
            LIRInstr mul{};
            mul.op = LIROp::Mul;
            mul.result = scaled;
            mul.type = TypeTable::I64;
            mul.bin.lhs = int_operand;
            mul.bin.rhs = scale;
            mul.loc = expr->loc;
            emit(mul);
            int_operand = scaled;
        }

        VReg r = freshVReg();
        LIRInstr i{};
        i.op = (expr->op == HIRBinOp::Add) ? LIROp::Add : LIROp::Sub;
        i.result = r;
        i.type = expr->type;
        i.bin.lhs = ptr_operand;
        i.bin.rhs = int_operand;
        i.loc = expr->loc;
        emit(i);
        return r;
    }

    bool is_float = ctx_.types.isFloat(expr->lhs->type);
    LIROp lir_op;

    switch (expr->op) {
        case HIRBinOp::Add:   lir_op = is_float ? LIROp::FAdd : LIROp::Add; break;
        case HIRBinOp::Sub:   lir_op = is_float ? LIROp::FSub : LIROp::Sub; break;
        case HIRBinOp::Mul:   lir_op = is_float ? LIROp::FMul : LIROp::Mul; break;
        case HIRBinOp::Div:   lir_op = is_float ? LIROp::FDiv : LIROp::Div; break;
        case HIRBinOp::Mod:   lir_op = LIROp::Mod; break;
        case HIRBinOp::BitAnd: lir_op = LIROp::BAnd; break;
        case HIRBinOp::BitOr:  lir_op = LIROp::BOr; break;
        case HIRBinOp::BitXor: lir_op = LIROp::BXor; break;
        case HIRBinOp::Shl:   lir_op = LIROp::Shl; break;
        case HIRBinOp::Shr:   lir_op = LIROp::Shr; break;
        case HIRBinOp::Eq:    lir_op = is_float ? LIROp::FCmpEq : LIROp::ICmpEq; break;
        case HIRBinOp::NotEq: lir_op = is_float ? LIROp::FCmpNe : LIROp::ICmpNe; break;
        case HIRBinOp::Lt:    lir_op = is_float ? LIROp::FCmpLt : LIROp::ICmpLt; break;
        case HIRBinOp::LtEq:  lir_op = is_float ? LIROp::FCmpLe : LIROp::ICmpLe; break;
        case HIRBinOp::Gt:    lir_op = is_float ? LIROp::FCmpGt : LIROp::ICmpGt; break;
        case HIRBinOp::GtEq:  lir_op = is_float ? LIROp::FCmpGe : LIROp::ICmpGe; break;
        default: return INVALID_VREG;
    }

    VReg r = freshVReg();
    LIRInstr i{};
    i.op = lir_op;
    i.result = r;
    i.type = expr->type;
    i.bin.lhs = lhs;
    i.bin.rhs = rhs;
    i.loc = expr->loc;
    emit(i);
    return r;
}

VReg LIRBuilder::lowerUnaryOp(const HIRUnaryOpExpr* expr) {
    VReg operand = lowerExpr(expr->operand);
    bool is_float = ctx_.types.isFloat(expr->operand->type);

    switch (expr->op) {
        case HIRUnaryOp::Deref: {
            VReg r = freshVReg();
            LIRInstr i{};
            i.op = LIROp::Load;
            i.result = r;
            i.type = expr->type;
            i.load.ptr = operand;
            i.loc = expr->loc;
            emit(i);
            return r;
        }
        case HIRUnaryOp::AddrOf:
        case HIRUnaryOp::AddrOfVar: {
            VReg r = freshVReg();
            LIRInstr i{};
            i.op = LIROp::AddrOf;
            i.result = r;
            i.type = expr->type;
            i.addr_of.source = operand;
            i.loc = expr->loc;
            emit(i);
            return r;
        }
        case HIRUnaryOp::Neg: {
            LIROp op = is_float ? LIROp::FNeg : LIROp::Neg;
            VReg r = freshVReg();
            LIRInstr i{};
            i.op = op;
            i.result = r;
            i.type = expr->type;
            i.unary.operand = operand;
            i.loc = expr->loc;
            emit(i);
            return r;
        }
        case HIRUnaryOp::Not: {
            VReg r = freshVReg();
            LIRInstr i{};
            i.op = LIROp::Not;
            i.result = r;
            i.type = expr->type;
            i.unary.operand = operand;
            i.loc = expr->loc;
            emit(i);
            return r;
        }
        case HIRUnaryOp::BitNot: {
            VReg r = freshVReg();
            LIRInstr i{};
            i.op = LIROp::BNot;  // bitwise complement (x86 NOT)
            i.result = r;
            i.type = expr->type;
            i.unary.operand = operand;
            i.loc = expr->loc;
            emit(i);
            return r;
        }
    }
    return INVALID_VREG;
}

VReg LIRBuilder::lowerCall(const HIRCallExpr* expr) {
    // Check for built-in atomic/fence/percpu intrinsics
    auto callee = expr->callee;

    // atomic_load(ptr, order) -> value
    if (callee == "atomic_load" && expr->arg_count == 2) {
        VReg ptr = lowerExpr(expr->args[0]);
        VReg order_vreg = lowerExpr(expr->args[1]);
        (void)order_vreg;  // order extracted as constant below
        VReg r = freshVReg();
        LIRInstr i{};
        i.op = LIROp::AtomicLoad;
        i.result = r;
        i.type = expr->type;
        i.atomic_load.ptr = ptr;
        i.atomic_load.order = MemOrder::SeqCst;  // default SeqCst
        i.loc = expr->loc;
        emit(i);
        return r;
    }

    // atomic_store(ptr, value, order)
    if (callee == "atomic_store" && expr->arg_count >= 2) {
        VReg ptr = lowerExpr(expr->args[0]);
        VReg val = lowerExpr(expr->args[1]);
        LIRInstr i{};
        i.op = LIROp::AtomicStore;
        i.result = INVALID_VREG;
        i.type = TypeTable::Unit;
        i.atomic_store.ptr = ptr;
        i.atomic_store.value = val;
        i.atomic_store.order = MemOrder::SeqCst;
        i.loc = expr->loc;
        emit(i);
        return INVALID_VREG;
    }

    // atomic_cas(ptr, expected, desired) -> old value
    if (callee == "atomic_cas" && expr->arg_count >= 3) {
        VReg ptr = lowerExpr(expr->args[0]);
        VReg expected = lowerExpr(expr->args[1]);
        VReg desired = lowerExpr(expr->args[2]);
        VReg r = freshVReg();
        LIRInstr i{};
        i.op = LIROp::AtomicCas;
        i.result = r;
        i.type = expr->type;
        i.atomic_cas.ptr = ptr;
        i.atomic_cas.expected = expected;
        i.atomic_cas.desired = desired;
        i.atomic_cas.order = MemOrder::SeqCst;
        i.loc = expr->loc;
        emit(i);
        return r;
    }

    // atomic_fetch_add(ptr, value) -> old value
    if (callee == "atomic_fetch_add" && expr->arg_count >= 2) {
        VReg ptr = lowerExpr(expr->args[0]);
        VReg val = lowerExpr(expr->args[1]);
        VReg r = freshVReg();
        LIRInstr i{};
        i.op = LIROp::AtomicFetchAdd;
        i.result = r;
        i.type = expr->type;
        i.atomic_fetch_add.ptr = ptr;
        i.atomic_fetch_add.value = val;
        i.atomic_fetch_add.order = MemOrder::SeqCst;
        i.loc = expr->loc;
        emit(i);
        return r;
    }

    // mfence() / sfence() / lfence() / compiler_barrier()
    if (callee == "mfence" && expr->arg_count == 0) {
        LIRInstr i{};
        i.op = LIROp::Fence;
        i.result = INVALID_VREG;
        i.type = TypeTable::Unit;
        i.fence.order = MemOrder::SeqCst;
        i.loc = expr->loc;
        emit(i);
        return INVALID_VREG;
    }
    if (callee == "sfence" && expr->arg_count == 0) {
        LIRInstr i{};
        i.op = LIROp::Fence;
        i.result = INVALID_VREG;
        i.type = TypeTable::Unit;
        i.fence.order = MemOrder::Release;
        i.loc = expr->loc;
        emit(i);
        return INVALID_VREG;
    }
    if (callee == "lfence" && expr->arg_count == 0) {
        LIRInstr i{};
        i.op = LIROp::Fence;
        i.result = INVALID_VREG;
        i.type = TypeTable::Unit;
        i.fence.order = MemOrder::Acquire;
        i.loc = expr->loc;
        emit(i);
        return INVALID_VREG;
    }
    if (callee == "compiler_barrier" && expr->arg_count == 0) {
        LIRInstr i{};
        i.op = LIROp::CompilerFence;
        i.result = INVALID_VREG;
        i.type = TypeTable::Unit;
        i.loc = expr->loc;
        emit(i);
        return INVALID_VREG;
    }

    // volatile_read(ptr) -> value
    if (callee == "volatile_read" && expr->arg_count == 1) {
        VReg ptr = lowerExpr(expr->args[0]);
        VReg r = freshVReg();
        LIRInstr i{};
        i.op = LIROp::Load;
        i.result = r;
        i.type = expr->type;
        i.load.ptr = ptr;
        i.load.is_volatile = true;
        i.loc = expr->loc;
        emit(i);
        return r;
    }

    // volatile_write(ptr, value)
    if (callee == "volatile_write" && expr->arg_count == 2) {
        VReg ptr = lowerExpr(expr->args[0]);
        VReg val = lowerExpr(expr->args[1]);
        LIRInstr i{};
        i.op = LIROp::Store;
        i.result = INVALID_VREG;
        i.type = expr->type;
        i.store.ptr = ptr;
        i.store.value = val;
        i.store.is_volatile = true;
        i.loc = expr->loc;
        emit(i);
        return INVALID_VREG;
    }

    // percpu_load(offset) -> value
    if (callee == "percpu_load" && expr->arg_count == 1) {
        VReg offset = lowerExpr(expr->args[0]);
        VReg r = freshVReg();
        LIRInstr i{};
        i.op = LIROp::PercpuLoad;
        i.result = r;
        i.type = expr->type;
        i.percpu_load.offset = offset;
        i.loc = expr->loc;
        emit(i);
        return r;
    }

    // percpu_store(offset, value)
    if (callee == "percpu_store" && expr->arg_count == 2) {
        VReg offset = lowerExpr(expr->args[0]);
        VReg val = lowerExpr(expr->args[1]);
        LIRInstr i{};
        i.op = LIROp::PercpuStore;
        i.result = INVALID_VREG;
        i.type = TypeTable::Unit;
        i.percpu_store.offset = offset;
        i.percpu_store.value = val;
        i.loc = expr->loc;
        emit(i);
        return INVALID_VREG;
    }

    // Regular call
    VReg* args = nullptr;
    if (expr->arg_count > 0) {
        args = ctx_.arena.makeArray<VReg>(expr->arg_count);
        for (uint32_t a = 0; a < expr->arg_count; ++a) {
            args[a] = lowerExpr(expr->args[a]);
        }
    }

    VReg r = (expr->type == TypeTable::Unit) ? INVALID_VREG : freshVReg();
    LIRInstr i{};
    i.op = LIROp::Call;
    i.result = r;
    i.type = expr->type;
    i.call.callee = expr->callee;
    i.call.args = args;
    i.call.arg_count = expr->arg_count;
    i.call.is_tail = expr->is_tail_call;
    i.loc = expr->loc;
    emit(i);
    return r;
}

VReg LIRBuilder::lowerFnRef(const HIRFnRefExpr* expr) {
    VReg r = freshVReg();
    LIRInstr i{};
    i.op = LIROp::FnRef;
    i.result = r;
    i.type = expr->type;
    i.fn_ref.fn_name = expr->fn_name;
    i.loc = expr->loc;
    emit(i);
    return r;
}

VReg LIRBuilder::lowerCallIndirect(const HIRCallIndirectExpr* expr) {
    VReg callee = lowerExpr(expr->callee);
    VReg* args = nullptr;
    if (expr->arg_count > 0) {
        args = ctx_.arena.makeArray<VReg>(expr->arg_count);
        for (uint32_t a = 0; a < expr->arg_count; ++a) {
            args[a] = lowerExpr(expr->args[a]);
        }
    }

    VReg r = (expr->type == TypeTable::Unit) ? INVALID_VREG : freshVReg();
    LIRInstr i{};
    i.op = LIROp::CallIndirect;
    i.result = r;
    i.type = expr->type;
    i.call_indirect.callee = callee;
    i.call_indirect.args = args;
    i.call_indirect.arg_count = expr->arg_count;
    i.loc = expr->loc;
    emit(i);
    return r;
}

VReg LIRBuilder::lowerIf(const HIRIfExpr* expr) {
    VReg cond = lowerExpr(expr->condition);

    // Phi slot for result (allocated before branching)
    VReg slot = INVALID_VREG;
    bool need_merge = (expr->type != TypeTable::Unit);
    if (need_merge) {
        slot = freshVReg();
        LIRInstr alloc{};
        alloc.op = LIROp::StructAlloc;
        alloc.result = slot;
        alloc.type = expr->type;
        alloc.struct_alloc.size = 8;
        alloc.struct_alloc.align = 8;
        alloc.loc = expr->loc;
        emit(alloc);
    }

    uint32_t then_bb = newBlock("if_then");
    uint32_t else_bb = newBlock("if_else");
    uint32_t merge_bb = newBlock("if_merge");

    emitCondBranch(cond, then_bb, else_bb);

    // Then branch
    switchToBlock(then_bb);
    VReg then_val = lowerExpr(expr->then_branch);
    uint32_t then_exit = current_block_;
    bool then_terminated = blockTerminated(blocks_[then_exit].instrs);
    if (!then_terminated) {
        if (need_merge && then_val != INVALID_VREG) {
            LIRInstr st{};
            st.op = LIROp::Store;
            st.result = INVALID_VREG;
            st.type = TypeTable::Unit;
            st.store.ptr = slot;
            st.store.value = then_val;
            emit(st);
        }
        emitBranch(merge_bb);
    }

    // Else branch
    switchToBlock(else_bb);
    VReg else_val = INVALID_VREG;
    if (expr->else_branch) {
        else_val = lowerExpr(expr->else_branch);
    }
    uint32_t else_exit = current_block_;
    bool else_terminated = blockTerminated(blocks_[else_exit].instrs);
    if (!else_terminated) {
        if (need_merge && else_val != INVALID_VREG) {
            LIRInstr st{};
            st.op = LIROp::Store;
            st.result = INVALID_VREG;
            st.type = TypeTable::Unit;
            st.store.ptr = slot;
            st.store.value = else_val;
            emit(st);
        }
        emitBranch(merge_bb);
    }

    switchToBlock(merge_bb);

    if (need_merge) {
        VReg result = freshVReg();
        LIRInstr load{};
        load.op = LIROp::Load;
        load.result = result;
        load.type = expr->type;
        load.load.ptr = slot;
        load.loc = expr->loc;
        emit(load);
        return result;
    }

    return INVALID_VREG;
}

VReg LIRBuilder::lowerMatch(const HIRMatchExpr* expr) {
    VReg scrutinee = lowerExpr(expr->scrutinee);
    TypeId scrut_type = expr->scrutinee->type;
    auto& type_info = ctx_.types.get(scrut_type);

    uint32_t merge_bb = newBlock("match_merge");

    VReg slot = INVALID_VREG;
    if (expr->type != TypeTable::Unit) {
        slot = freshVReg();
        LIRInstr alloc{};
        alloc.op = LIROp::StructAlloc;
        alloc.result = slot;
        alloc.type = expr->type;
        alloc.struct_alloc.size = 8;
        alloc.struct_alloc.align = 8;
        alloc.loc = expr->loc;
        emit(alloc);
    }

    for (uint32_t a = 0; a < expr->arm_count; ++a) {
        auto& arm = expr->arms[a];
        bool is_last = (a + 1 == expr->arm_count);

        uint32_t body_bb = newBlock("match_arm");
        // Need a next_bb even for last arm if it has a guard (guard can fail)
        uint32_t next_bb = (is_last && !arm.guard) ? merge_bb : newBlock("match_next");

        switch (arm.pattern->kind) {
            case HIRPattern::Kind::IntLit: {
                auto* pat = static_cast<const HIRIntLitPattern*>(arm.pattern);
                VReg pat_val = freshVReg();
                LIRInstr ci{};
                ci.op = LIROp::ConstInt;
                ci.result = pat_val;
                ci.type = scrut_type;
                ci.const_int.value = pat->value;
                emit(ci);

                VReg cmp = freshVReg();
                LIRInstr cmp_i{};
                cmp_i.op = LIROp::ICmpEq;
                cmp_i.result = cmp;
                cmp_i.type = TypeTable::Bool;
                cmp_i.bin.lhs = scrutinee;
                cmp_i.bin.rhs = pat_val;
                emit(cmp_i);

                emitCondBranch(cmp, body_bb, next_bb);
                break;
            }
            case HIRPattern::Kind::BoolLit: {
                auto* pat = static_cast<const HIRBoolLitPattern*>(arm.pattern);
                VReg pat_val = freshVReg();
                LIRInstr ci{};
                ci.op = LIROp::ConstBool;
                ci.result = pat_val;
                ci.type = TypeTable::Bool;
                ci.const_bool.value = pat->value;
                emit(ci);

                VReg cmp = freshVReg();
                LIRInstr cmp_i{};
                cmp_i.op = LIROp::ICmpEq;
                cmp_i.result = cmp;
                cmp_i.type = TypeTable::Bool;
                cmp_i.bin.lhs = scrutinee;
                cmp_i.bin.rhs = pat_val;
                emit(cmp_i);

                emitCondBranch(cmp, body_bb, next_bb);
                break;
            }
            case HIRPattern::Kind::Wildcard: {
                emitBranch(body_bb);
                break;
            }
            case HIRPattern::Kind::Variable: {
                auto* pat = static_cast<const HIRVariablePattern*>(arm.pattern);
                locals_[pat->name] = scrutinee;
                emitBranch(body_bb);
                break;
            }
            case HIRPattern::Kind::Enum: {
                auto* pat = static_cast<const HIREnumPattern*>(arm.pattern);
                int64_t tag = enumVariantValue(scrut_type, pat->variant_name);
                VReg tag_val = freshVReg();
                LIRInstr ci{};
                ci.op = LIROp::ConstInt;
                ci.result = tag_val;
                ci.type = scrut_type;
                ci.const_int.value = tag;
                emit(ci);

                VReg cmp = freshVReg();
                LIRInstr cmp_i{};
                cmp_i.op = LIROp::ICmpEq;
                cmp_i.result = cmp;
                cmp_i.type = TypeTable::Bool;
                cmp_i.bin.lhs = scrutinee;
                cmp_i.bin.rhs = tag_val;
                emit(cmp_i);

                emitCondBranch(cmp, body_bb, next_bb);
                break;
            }
            case HIRPattern::Kind::Union: {
                auto* pat = static_cast<const HIRUnionPattern*>(arm.pattern);
                uint32_t tag = unionVariantTag(scrut_type, pat->variant_name);

                // Load tag from scrutinee (offset 0)
                VReg tag_ptr = freshVReg();
                LIRInstr fp{};
                fp.op = LIROp::FieldPtr;
                fp.result = tag_ptr;
                fp.type = ctx_.types.makePtr(TypeTable::I64, false);
                fp.field_ptr.base = scrutinee;
                fp.field_ptr.offset = 0;
                emit(fp);

                VReg actual_tag = freshVReg();
                LIRInstr ld{};
                ld.op = LIROp::Load;
                ld.result = actual_tag;
                ld.type = TypeTable::I64;
                ld.load.ptr = tag_ptr;
                emit(ld);

                VReg expected_tag = freshVReg();
                LIRInstr ci{};
                ci.op = LIROp::ConstInt;
                ci.result = expected_tag;
                ci.type = TypeTable::I64;
                ci.const_int.value = static_cast<int64_t>(tag);
                emit(ci);

                VReg cmp = freshVReg();
                LIRInstr cmp_i{};
                cmp_i.op = LIROp::ICmpEq;
                cmp_i.result = cmp;
                cmp_i.type = TypeTable::Bool;
                cmp_i.bin.lhs = actual_tag;
                cmp_i.bin.rhs = expected_tag;
                emit(cmp_i);

                emitCondBranch(cmp, body_bb, next_bb);

                // Bind inner pattern (payload extraction) in body block
                if (pat->inner && pat->inner->kind == HIRPattern::Kind::Variable) {
                    auto* var_pat = static_cast<const HIRVariablePattern*>(pat->inner);
                    switchToBlock(body_bb);

                    VReg payload_ptr = freshVReg();
                    LIRInstr fp2{};
                    fp2.op = LIROp::FieldPtr;
                    fp2.result = payload_ptr;
                    TypeId payload_type = type_info.union_.variants[tag].payload_type;
                    fp2.type = ctx_.types.makePtr(payload_type, false);
                    fp2.field_ptr.base = scrutinee;
                    fp2.field_ptr.offset = 8;
                    emit(fp2);

                    VReg payload_val = freshVReg();
                    LIRInstr ld2{};
                    ld2.op = LIROp::Load;
                    ld2.result = payload_val;
                    ld2.type = payload_type;
                    ld2.load.ptr = payload_ptr;
                    emit(ld2);

                    locals_[var_pat->name] = payload_val;

                    // Evaluate guard if present
                    if (arm.guard) {
                        VReg guard_val = lowerExpr(arm.guard);
                        uint32_t real_body_bb = newBlock("match_guard_pass");
                        emitCondBranch(guard_val, real_body_bb, next_bb);
                        switchToBlock(real_body_bb);
                    }

                    VReg arm_val = lowerExpr(arm.body);
                    uint32_t arm_exit = current_block_;
                    bool arm_terminated = blockTerminated(blocks_[arm_exit].instrs);
                    if (!arm_terminated) {
                        if (slot != INVALID_VREG && arm_val != INVALID_VREG) {
                            LIRInstr st{};
                            st.op = LIROp::Store;
                            st.result = INVALID_VREG;
                            st.type = TypeTable::Unit;
                            st.store.ptr = slot;
                            st.store.value = arm_val;
                            emit(st);
                        }
                        emitBranch(merge_bb);
                    }

                    if (!is_last || arm.guard) {
                        switchToBlock(next_bb);
                        if (is_last && arm.guard) {
                            emitBranch(merge_bb);
                        }
                    }
                    continue;
                }
                break;
            }
        }

        // Default: lower arm body
        switchToBlock(body_bb);

        // Evaluate guard if present
        if (arm.guard) {
            VReg guard_val = lowerExpr(arm.guard);
            uint32_t real_body_bb = newBlock("match_guard_pass");
            emitCondBranch(guard_val, real_body_bb, next_bb);
            switchToBlock(real_body_bb);
        }

        VReg arm_val = lowerExpr(arm.body);
        uint32_t arm_exit = current_block_;
        bool arm_terminated = blockTerminated(blocks_[arm_exit].instrs);

        if (!arm_terminated) {
            if (slot != INVALID_VREG && arm_val != INVALID_VREG) {
                LIRInstr st{};
                st.op = LIROp::Store;
                st.result = INVALID_VREG;
                st.type = TypeTable::Unit;
                st.store.ptr = slot;
                st.store.value = arm_val;
                emit(st);
            }
            emitBranch(merge_bb);
        }

        if (!is_last || arm.guard) {
            switchToBlock(next_bb);
            // If this is the last arm with a guard, the guard-fail path
            // needs to reach merge_bb (no more arms to try)
            if (is_last && arm.guard) {
                emitBranch(merge_bb);
            }
        }
    }

    switchToBlock(merge_bb);

    if (slot != INVALID_VREG) {
        VReg result = freshVReg();
        LIRInstr load{};
        load.op = LIROp::Load;
        load.result = result;
        load.type = expr->type;
        load.load.ptr = slot;
        load.loc = expr->loc;
        emit(load);
        return result;
    }

    return INVALID_VREG;
}

VReg LIRBuilder::lowerBlock(const HIRBlockExpr* expr) {
    for (uint32_t i = 0; i < expr->stmt_count; ++i) {
        lowerStmt(expr->stmts[i]);
    }
    if (expr->result) return lowerExpr(expr->result);
    return INVALID_VREG;
}

VReg LIRBuilder::lowerReturn(const HIRReturnExpr* expr) {
    VReg val = INVALID_VREG;
    if (expr->value) val = lowerExpr(expr->value);
    LIRInstr i{};
    i.op = LIROp::Ret;
    i.result = INVALID_VREG;
    i.type = expr->type;
    i.ret.value = val;
    i.loc = expr->loc;
    emit(i);
    return INVALID_VREG;
}

VReg LIRBuilder::lowerStructLit(const HIRStructLitExpr* expr) {
    uint32_t size = structSize(expr->type);
    uint32_t align = structAlign(expr->type);

    VReg base = freshVReg();
    LIRInstr alloc{};
    alloc.op = LIROp::StructAlloc;
    alloc.result = base;
    alloc.type = expr->type;
    alloc.struct_alloc.size = size;
    alloc.struct_alloc.align = align;
    alloc.loc = expr->loc;
    emit(alloc);

    auto& ti = ctx_.types.get(expr->type);
    for (uint32_t f = 0; f < expr->field_count; ++f) {
        VReg val = lowerExpr(expr->fields[f].value);
        uint32_t offset = structFieldOffset(expr->type, expr->fields[f].name);

        TypeId field_type = TypeTable::I64;
        if (ti.kind == TypeKind::Struct) {
            bool found = false;
            for (uint32_t fi = 0; fi < ti.struct_.field_count; ++fi) {
                if (ti.struct_.fields[fi].name == expr->fields[f].name) {
                    field_type = ti.struct_.fields[fi].type;
                    found = true;
                    break;
                }
            }
            assert(found && "struct field not found — should be caught by HIRBuilder");
        }

        VReg fp = freshVReg();
        LIRInstr fp_instr{};
        fp_instr.op = LIROp::FieldPtr;
        fp_instr.result = fp;
        fp_instr.type = ctx_.types.makePtr(field_type, false);
        fp_instr.field_ptr.base = base;
        fp_instr.field_ptr.offset = offset;
        fp_instr.loc = expr->fields[f].loc;
        emit(fp_instr);

        LIRInstr store{};
        store.op = LIROp::Store;
        store.result = INVALID_VREG;
        store.type = TypeTable::Unit;
        store.store.ptr = fp;
        store.store.value = val;
        emit(store);
    }

    return base;
}

VReg LIRBuilder::lowerFieldAccess(const HIRFieldAccessExpr* expr) {
    VReg obj = lowerExpr(expr->object);
    uint32_t offset = structFieldOffset(expr->object->type, expr->field_name);

    VReg fp = freshVReg();
    LIRInstr fp_instr{};
    fp_instr.op = LIROp::FieldPtr;
    fp_instr.result = fp;
    fp_instr.type = ctx_.types.makePtr(expr->type, false);
    fp_instr.field_ptr.base = obj;
    fp_instr.field_ptr.offset = offset;
    fp_instr.loc = expr->loc;
    emit(fp_instr);

    VReg r = freshVReg();
    LIRInstr load{};
    load.op = LIROp::Load;
    load.result = r;
    load.type = expr->type;
    load.load.ptr = fp;
    load.loc = expr->loc;
    emit(load);
    return r;
}

VReg LIRBuilder::lowerEnumAccess(const HIREnumAccessExpr* expr) {
    int64_t value = enumVariantValue(expr->type, expr->variant_name);
    VReg r = freshVReg();
    LIRInstr i{};
    i.op = LIROp::ConstInt;
    i.result = r;
    i.type = expr->type;
    i.const_int.value = value;
    i.loc = expr->loc;
    emit(i);
    return r;
}

VReg LIRBuilder::lowerUnionVariant(const HIRUnionVariantExpr* expr) {
    uint32_t size = structSize(expr->type);
    uint32_t align = structAlign(expr->type);

    VReg base = freshVReg();
    LIRInstr alloc{};
    alloc.op = LIROp::StructAlloc;
    alloc.result = base;
    alloc.type = expr->type;
    alloc.struct_alloc.size = size;
    alloc.struct_alloc.align = align;
    alloc.loc = expr->loc;
    emit(alloc);

    uint32_t tag = unionVariantTag(expr->type, expr->variant_name);
    VReg tag_val = freshVReg();
    LIRInstr ci{};
    ci.op = LIROp::ConstInt;
    ci.result = tag_val;
    ci.type = TypeTable::I64;
    ci.const_int.value = static_cast<int64_t>(tag);
    emit(ci);

    VReg tag_ptr = freshVReg();
    LIRInstr fp{};
    fp.op = LIROp::FieldPtr;
    fp.result = tag_ptr;
    fp.type = ctx_.types.makePtr(TypeTable::I64, false);
    fp.field_ptr.base = base;
    fp.field_ptr.offset = 0;
    emit(fp);

    LIRInstr tag_store{};
    tag_store.op = LIROp::Store;
    tag_store.result = INVALID_VREG;
    tag_store.type = TypeTable::Unit;
    tag_store.store.ptr = tag_ptr;
    tag_store.store.value = tag_val;
    emit(tag_store);

    if (expr->payload) {
        VReg payload_val = lowerExpr(expr->payload);

        VReg payload_ptr = freshVReg();
        LIRInstr fp2{};
        fp2.op = LIROp::FieldPtr;
        fp2.result = payload_ptr;
        fp2.type = ctx_.types.makePtr(expr->payload->type, false);
        fp2.field_ptr.base = base;
        fp2.field_ptr.offset = 8;
        emit(fp2);

        LIRInstr payload_store{};
        payload_store.op = LIROp::Store;
        payload_store.result = INVALID_VREG;
        payload_store.type = TypeTable::Unit;
        payload_store.store.ptr = payload_ptr;
        payload_store.store.value = payload_val;
        emit(payload_store);
    }

    return base;
}

VReg LIRBuilder::lowerAddrOf(const HIRAddrOfExpr* expr) {
    // For &x / &var x where x is a var binding, return the var's stack address
    // directly instead of loading the value and taking address of the copy.
    if (expr->operand->kind == HIRExpr::Kind::Ident) {
        auto* ident = static_cast<const HIRIdentExpr*>(expr->operand);
        auto var_it = var_addrs_.find(ident->name);
        if (var_it != var_addrs_.end()) {
            // The var_addr IS the pointer to the stack slot — just return it
            return var_it->second;
        }
    }

    // For non-variable expressions, lower normally and wrap in addr_of
    VReg operand = lowerExpr(expr->operand);
    VReg r = freshVReg();
    LIRInstr i{};
    i.op = LIROp::AddrOf;
    i.result = r;
    i.type = expr->type;
    i.addr_of.source = operand;
    i.loc = expr->loc;
    emit(i);
    return r;
}

VReg LIRBuilder::lowerDeref(const HIRDerefExpr* expr) {
    VReg ptr = lowerExpr(expr->operand);
    VReg r = freshVReg();
    LIRInstr i{};
    i.op = LIROp::Load;
    i.result = r;
    i.type = expr->type;
    i.load.ptr = ptr;
    i.loc = expr->loc;
    emit(i);
    return r;
}

VReg LIRBuilder::lowerCast(const HIRCastExpr* expr) {
    VReg operand = lowerExpr(expr->operand);
    VReg r = freshVReg();
    LIRInstr i{};
    i.op = LIROp::Cast;
    i.result = r;
    i.type = expr->type;
    i.cast.operand = operand;
    i.cast.src_type = expr->operand->type;
    i.loc = expr->loc;
    emit(i);
    return r;
}

// ============================================================================
// Loop / Break / Continue lowering
// ============================================================================

VReg LIRBuilder::lowerLoop(const HIRLoopExpr* expr) {
    // Loop with accumulator pattern:
    // 1. Evaluate init values
    // 2. Create header block with parameters (accumulators)
    // 3. Lower body in header block context
    // 4. Break -> branch to exit block with value
    // 5. Continue -> branch back to header with new acc values

    // Save current loop context
    auto saved_loop_header = current_loop_header_;
    auto saved_loop_exit = current_loop_exit_;
    auto saved_loop_result = current_loop_result_;

    // Evaluate initial values
    std::vector<VReg> init_vals;
    for (uint32_t i = 0; i < expr->binding_count; ++i) {
        init_vals.push_back(lowerExpr(expr->bindings[i].init));
    }

    // Create header and exit blocks
    uint32_t header_bb = newBlock("loop_header");
    uint32_t exit_bb = newBlock("loop_exit");

    // Set block parameter types for header
    for (uint32_t i = 0; i < expr->binding_count; ++i) {
        blocks_[header_bb].param_types.push_back(expr->bindings[i].type);
    }

    // Result vreg (allocated in exit block)
    VReg result = freshVReg();
    blocks_[exit_bb].param_types.push_back(expr->type);

    // Branch to header with initial values
    emitBranchWithArgs(header_bb, init_vals);

    // Switch to header block
    switchToBlock(header_bb);

    // Bind loop variables to block args
    current_loop_header_ = header_bb;
    current_loop_exit_ = exit_bb;
    current_loop_result_ = result;

    for (uint32_t i = 0; i < expr->binding_count; ++i) {
        VReg arg_vreg = freshVReg();
        LIRInstr arg_i{};
        arg_i.op = LIROp::BlockArg;
        arg_i.result = arg_vreg;
        arg_i.type = expr->bindings[i].type;
        arg_i.block_arg.index = i;
        arg_i.loc = expr->bindings[i].loc;
        emit(arg_i);
        locals_[expr->bindings[i].name] = arg_vreg;
    }

    // Lower body
    lowerExpr(expr->body);

    // Switch to exit block
    switchToBlock(exit_bb);

    // Load the result from block arg
    LIRInstr exit_arg{};
    exit_arg.op = LIROp::BlockArg;
    exit_arg.result = result;
    exit_arg.type = expr->type;
    exit_arg.block_arg.index = 0;
    exit_arg.loc = expr->loc;
    emit(exit_arg);

    // Restore loop context
    current_loop_header_ = saved_loop_header;
    current_loop_exit_ = saved_loop_exit;
    current_loop_result_ = saved_loop_result;

    return result;
}

VReg LIRBuilder::lowerBreak(const HIRBreakExpr* expr) {
    std::vector<VReg> args;
    if (expr->value) {
        args.push_back(lowerExpr(expr->value));
    }
    // Branch to loop exit block, passing the break value as block arg
    emitBranchWithArgs(current_loop_exit_, args);
    return args.empty() ? INVALID_VREG : args[0];
}

VReg LIRBuilder::lowerContinue(const HIRContinueExpr* expr) {
    // Lower continue args (new accumulator values)
    std::vector<VReg> args;
    for (uint32_t i = 0; i < expr->arg_count; ++i) {
        args.push_back(lowerExpr(expr->args[i]));
    }
    // Branch back to loop header with new accumulator values
    emitBranchWithArgs(current_loop_header_, args);
    return INVALID_VREG;
}

// ============================================================================
// Statement lowering
// ============================================================================

void LIRBuilder::lowerStmt(const HIRStmt* stmt) {
    switch (stmt->kind) {
        case HIRStmt::Kind::ValDecl: {
            auto* s = static_cast<const HIRValDeclStmt*>(stmt);
            VReg val = lowerExpr(s->init);
            locals_[s->name] = val;
            break;
        }
        case HIRStmt::Kind::VarDecl: {
            auto* s = static_cast<const HIRVarDeclStmt*>(stmt);
            VReg init_val = lowerExpr(s->init);

            VReg slot = freshVReg();
            LIRInstr alloc{};
            alloc.op = LIROp::StructAlloc;
            alloc.result = slot;
            alloc.type = s->type;
            alloc.struct_alloc.size = 8;
            alloc.struct_alloc.align = 8;
            alloc.loc = s->loc;
            emit(alloc);

            LIRInstr store{};
            store.op = LIROp::Store;
            store.result = INVALID_VREG;
            store.type = TypeTable::Unit;
            store.store.ptr = slot;
            store.store.value = init_val;
            emit(store);

            var_addrs_[s->name] = slot;
            break;
        }
        case HIRStmt::Kind::ExprStmt: {
            auto* s = static_cast<const HIRExprStmt*>(stmt);
            lowerExpr(s->expr);
            break;
        }
        case HIRStmt::Kind::Assign: {
            auto* s = static_cast<const HIRAssignStmt*>(stmt);
            VReg val = lowerExpr(s->value);
            auto it = var_addrs_.find(s->name);
            if (it != var_addrs_.end()) {
                LIRInstr store{};
                store.op = LIROp::Store;
                store.result = INVALID_VREG;
                store.type = TypeTable::Unit;
                store.store.ptr = it->second;
                store.store.value = val;
                store.loc = s->loc;
                emit(store);
            }
            break;
        }
        case HIRStmt::Kind::FieldAssign: {
            auto* s = static_cast<const HIRFieldAssignStmt*>(stmt);
            if (s->target->kind == HIRExpr::Kind::FieldAccess) {
                auto* fa = static_cast<const HIRFieldAccessExpr*>(s->target);
                VReg obj = lowerExpr(fa->object);
                uint32_t offset = structFieldOffset(fa->object->type, fa->field_name);
                VReg val = lowerExpr(s->value);

                VReg fp = freshVReg();
                LIRInstr fp_instr{};
                fp_instr.op = LIROp::FieldPtr;
                fp_instr.result = fp;
                fp_instr.type = ctx_.types.makePtr(fa->type, true);
                fp_instr.field_ptr.base = obj;
                fp_instr.field_ptr.offset = offset;
                fp_instr.loc = s->loc;
                emit(fp_instr);

                LIRInstr store{};
                store.op = LIROp::Store;
                store.result = INVALID_VREG;
                store.type = TypeTable::Unit;
                store.store.ptr = fp;
                store.store.value = val;
                store.loc = s->loc;
                emit(store);
            }
            break;
        }
        case HIRStmt::Kind::DerefAssign: {
            auto* s = static_cast<const HIRDerefAssignStmt*>(stmt);
            // Unwrap deref to get the pointer address (not the loaded value)
            VReg ptr;
            if (s->target->kind == HIRExpr::Kind::Deref) {
                auto* deref = static_cast<const HIRDerefExpr*>(s->target);
                ptr = lowerExpr(deref->operand);
            } else {
                ptr = lowerExpr(s->target);
            }
            VReg val = lowerExpr(s->value);
            LIRInstr store{};
            store.op = LIROp::Store;
            store.result = INVALID_VREG;
            store.type = TypeTable::Unit;
            store.store.ptr = ptr;
            store.store.value = val;
            store.loc = s->loc;
            emit(store);
            break;
        }
        case HIRStmt::Kind::IndexAssign: {
            auto* s = static_cast<const HIRIndexAssignStmt*>(stmt);
            VReg base = lowerExpr(s->array);
            VReg idx = lowerExpr(s->index);
            VReg val = lowerExpr(s->value);

            // Get element size from array type
            TypeId arr_type = s->array->type;
            uint32_t elem_size = 8; // default
            if (arr_type < ctx_.types.size()) {
                const auto& ti = ctx_.types.get(arr_type);
                if (ti.kind == TypeKind::Array) {
                    elem_size = ctx_.types.sizeOf(ti.array.element);
                }
            }

            // offset = idx * elem_size
            VReg offset_vreg = freshVReg();
            LIRInstr mul{};
            mul.op = LIROp::Mul;
            mul.result = offset_vreg;
            mul.type = TypeTable::I64;
            mul.bin.lhs = idx;
            // Create a constant for elem_size
            VReg es = freshVReg();
            LIRInstr es_instr{};
            es_instr.op = LIROp::ConstInt;
            es_instr.result = es;
            es_instr.type = TypeTable::I64;
            es_instr.const_int.value = static_cast<int64_t>(elem_size);
            emit(es_instr);
            mul.bin.rhs = es;
            mul.loc = s->loc;
            emit(mul);

            // ptr = base + offset (FieldPtr with dynamic offset via Add)
            VReg elem_ptr = freshVReg();
            LIRInstr add{};
            add.op = LIROp::Add;
            add.result = elem_ptr;
            add.type = TypeTable::I64;
            add.bin.lhs = base;
            add.bin.rhs = offset_vreg;
            add.loc = s->loc;
            emit(add);

            // Store value at elem_ptr
            LIRInstr store{};
            store.op = LIROp::Store;
            store.result = INVALID_VREG;
            store.type = TypeTable::Unit;
            store.store.ptr = elem_ptr;
            store.store.value = val;
            store.loc = s->loc;
            emit(store);
            break;
        }
    }
}

// ============================================================================
// ArrayLit / IndexAccess / InlineAsm
// ============================================================================

VReg LIRBuilder::lowerArrayLit(const HIRArrayLitExpr* expr) {
    // Get element type and size from the array type
    TypeId arr_type = expr->type;
    uint32_t elem_size = 8; // default
    if (arr_type < ctx_.types.size()) {
        const auto& ti = ctx_.types.get(arr_type);
        if (ti.kind == TypeKind::Array) {
            elem_size = ctx_.types.sizeOf(ti.array.element);
        }
    }

    uint32_t total_size = elem_size * expr->element_count;
    if (total_size < 8) total_size = 8; // minimum allocation

    // StructAlloc for the array
    VReg base = freshVReg();
    LIRInstr alloc{};
    alloc.op = LIROp::StructAlloc;
    alloc.result = base;
    alloc.type = expr->type;
    alloc.struct_alloc.size = total_size;
    alloc.struct_alloc.align = 8;
    alloc.loc = expr->loc;
    emit(alloc);

    // FieldStore for each element: base + elem_size * index
    for (uint32_t i = 0; i < expr->element_count; ++i) {
        VReg val = lowerExpr(expr->elements[i]);
        uint32_t offset = elem_size * i;

        VReg fp = freshVReg();
        LIRInstr fp_instr{};
        fp_instr.op = LIROp::FieldPtr;
        fp_instr.result = fp;
        fp_instr.type = TypeTable::I64;
        fp_instr.field_ptr.base = base;
        fp_instr.field_ptr.offset = offset;
        fp_instr.loc = expr->loc;
        emit(fp_instr);

        LIRInstr store{};
        store.op = LIROp::Store;
        store.result = INVALID_VREG;
        store.type = TypeTable::Unit;
        store.store.ptr = fp;
        store.store.value = val;
        store.loc = expr->loc;
        emit(store);
    }

    return base;
}

VReg LIRBuilder::lowerIndexAccess(const HIRIndexAccessExpr* expr) {
    VReg base = lowerExpr(expr->array);
    VReg idx = lowerExpr(expr->index);

    // Get element type and size from array type
    TypeId arr_type = expr->array->type;
    uint32_t elem_size = 8; // default
    if (arr_type < ctx_.types.size()) {
        const auto& ti = ctx_.types.get(arr_type);
        if (ti.kind == TypeKind::Array) {
            elem_size = ctx_.types.sizeOf(ti.array.element);
        }
    }

    // offset = idx * elem_size
    VReg es = freshVReg();
    LIRInstr es_instr{};
    es_instr.op = LIROp::ConstInt;
    es_instr.result = es;
    es_instr.type = TypeTable::I64;
    es_instr.const_int.value = static_cast<int64_t>(elem_size);
    emit(es_instr);

    VReg offset_vreg = freshVReg();
    LIRInstr mul{};
    mul.op = LIROp::Mul;
    mul.result = offset_vreg;
    mul.type = TypeTable::I64;
    mul.bin.lhs = idx;
    mul.bin.rhs = es;
    mul.loc = expr->loc;
    emit(mul);

    // ptr = base + offset
    VReg elem_ptr = freshVReg();
    LIRInstr add{};
    add.op = LIROp::Add;
    add.result = elem_ptr;
    add.type = TypeTable::I64;
    add.bin.lhs = base;
    add.bin.rhs = offset_vreg;
    add.loc = expr->loc;
    emit(add);

    // Load value at elem_ptr
    VReg result = freshVReg();
    LIRInstr load{};
    load.op = LIROp::Load;
    load.result = result;
    load.type = expr->type;
    load.load.ptr = elem_ptr;
    load.loc = expr->loc;
    emit(load);

    return result;
}

VReg LIRBuilder::lowerInlineAsm(const HIRInlineAsmExpr* expr) {
    // Copy lines into arena-allocated arrays
    auto** lines = ctx_.arena.makeArray<const char*>(expr->line_count);
    auto* lengths = ctx_.arena.makeArray<uint32_t>(expr->line_count);
    for (uint32_t i = 0; i < expr->line_count; ++i) {
        lines[i] = expr->lines[i].text;
        lengths[i] = expr->lines[i].length;
    }

    VReg result = freshVReg();
    LIRInstr instr{};
    instr.op = LIROp::InlineAsm;
    instr.result = result;
    instr.type = TypeTable::Unit;
    instr.inline_asm.lines = lines;
    instr.inline_asm.line_lengths = lengths;
    instr.inline_asm.line_count = expr->line_count;
    instr.loc = expr->loc;
    emit(instr);

    return result;
}

// ============================================================================
// And/Or short-circuit (phi-slot pattern)
// ============================================================================

VReg LIRBuilder::lowerAndOr(VReg lhs, const HIRExpr* rhs_expr, bool is_and,
                             SourceLocation loc) {
    VReg slot = freshVReg();
    LIRInstr alloc{};
    alloc.op = LIROp::StructAlloc;
    alloc.result = slot;
    alloc.type = TypeTable::Bool;
    alloc.struct_alloc.size = 8;
    alloc.struct_alloc.align = 8;
    alloc.loc = loc;
    emit(alloc);

    uint32_t eval_bb = newBlock(is_and ? "and_eval" : "or_eval");
    uint32_t short_bb = newBlock(is_and ? "and_short" : "or_short");
    uint32_t merge_bb = newBlock(is_and ? "and_merge" : "or_merge");

    if (is_and) {
        emitCondBranch(lhs, eval_bb, short_bb);
    } else {
        emitCondBranch(lhs, short_bb, eval_bb);
    }

    // Evaluate rhs
    switchToBlock(eval_bb);
    VReg rhs = lowerExpr(rhs_expr);
    LIRInstr store_rhs{};
    store_rhs.op = LIROp::Store;
    store_rhs.result = INVALID_VREG;
    store_rhs.type = TypeTable::Unit;
    store_rhs.store.ptr = slot;
    store_rhs.store.value = rhs;
    emit(store_rhs);
    emitBranch(merge_bb);

    // Short-circuit: store lhs directly (and: false, or: true)
    switchToBlock(short_bb);
    LIRInstr store_short{};
    store_short.op = LIROp::Store;
    store_short.result = INVALID_VREG;
    store_short.type = TypeTable::Unit;
    store_short.store.ptr = slot;
    store_short.store.value = lhs;
    emit(store_short);
    emitBranch(merge_bb);

    // Merge: load result
    switchToBlock(merge_bb);
    VReg result = freshVReg();
    LIRInstr load{};
    load.op = LIROp::Load;
    load.result = result;
    load.type = TypeTable::Bool;
    load.load.ptr = slot;
    load.loc = loc;
    emit(load);

    return result;
}

} // namespace kern
