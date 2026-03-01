#include "kern/lir/LIRBuilder.h"
#include <cassert>
#include <cstring>
#include <sstream>

namespace kern {

LIRBuilder::LIRBuilder(CompilationContext& ctx) : ctx_(ctx) {}

VReg LIRBuilder::freshVReg() {
    return next_vreg_++;
}

void LIRBuilder::emit(LIRInstr instr) {
    blocks_[current_block_].instrs.push_back(instr);
}

uint32_t LIRBuilder::newBlock(std::string_view label) {
    uint32_t idx = static_cast<uint32_t>(blocks_.size());
    blocks_.push_back(BlockBuild{label, {}, {}});
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

    bool is_float = ctx_.types.isFloat(expr->lhs->type);
    LIROp lir_op;

    switch (expr->op) {
        case HIRBinOp::Add:   lir_op = is_float ? LIROp::FAdd : LIROp::Add; break;
        case HIRBinOp::Sub:   lir_op = is_float ? LIROp::FSub : LIROp::Sub; break;
        case HIRBinOp::Mul:   lir_op = is_float ? LIROp::FMul : LIROp::Mul; break;
        case HIRBinOp::Div:   lir_op = is_float ? LIROp::FDiv : LIROp::Div; break;
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
    }
    return INVALID_VREG;
}

VReg LIRBuilder::lowerCall(const HIRCallExpr* expr) {
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
    bool then_returned = !blocks_[then_exit].instrs.empty() &&
                         blocks_[then_exit].instrs.back().op == LIROp::Ret;
    if (!then_returned) {
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
    bool else_returned = !blocks_[else_exit].instrs.empty() &&
                         blocks_[else_exit].instrs.back().op == LIROp::Ret;
    if (!else_returned) {
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
        uint32_t next_bb = is_last ? merge_bb : newBlock("match_next");

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

                    VReg arm_val = lowerExpr(arm.body);
                    uint32_t arm_exit = current_block_;
                    bool arm_returned = !blocks_[arm_exit].instrs.empty() &&
                                        blocks_[arm_exit].instrs.back().op == LIROp::Ret;
                    if (!arm_returned) {
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

                    if (!is_last) switchToBlock(next_bb);
                    continue;
                }
                break;
            }
        }

        // Default: lower arm body
        switchToBlock(body_bb);
        VReg arm_val = lowerExpr(arm.body);
        uint32_t arm_exit = current_block_;
        bool arm_returned = !blocks_[arm_exit].instrs.empty() &&
                            blocks_[arm_exit].instrs.back().op == LIROp::Ret;

        if (!arm_returned) {
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

        if (!is_last) switchToBlock(next_bb);
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
            for (uint32_t fi = 0; fi < ti.struct_.field_count; ++fi) {
                if (ti.struct_.fields[fi].name == expr->fields[f].name) {
                    field_type = ti.struct_.fields[fi].type;
                    break;
                }
            }
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
            VReg ptr = lowerExpr(s->target);
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
    }
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
