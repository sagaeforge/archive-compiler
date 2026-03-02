#include "kern/lir/LIRBuilder.h"
#include <cassert>
#include <cstring>
#include <sstream>
#include <unordered_set>

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

static const FieldInfo* lookupField(const TypeTable& types, TypeId struct_type,
                                     std::string_view field_name) {
    auto& ti = types.get(struct_type);
    if (ti.kind == TypeKind::Struct) {
        for (uint32_t i = 0; i < ti.struct_.field_count; ++i) {
            if (ti.struct_.fields[i].name == field_name)
                return &ti.struct_.fields[i];
        }
    }
    return nullptr;
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
    mod->module_name = hir->module_name;

    // Pre-register global variable names so lowerIdent can find them during fn building
    for (uint32_t i = 0; i < hir->global_count; ++i) {
        // Use a placeholder index; real index assigned below when GlobalData is created
        global_label_map_[hir->globals[i]->name] = i;
    }

    // Pre-register vtable labels so lowerIdent can resolve them during fn building
    for (uint32_t i = 0; i < hir->vtable_count; ++i) {
        global_label_map_[hir->vtables[i].label] = hir->global_count + i;
        vtable_labels_.insert(hir->vtables[i].label);
    }

    // Pre-scan for variadic functions so lowerCall can set is_variadic flag
    for (uint32_t i = 0; i < hir->fn_count; ++i) {
        if (hir->functions[i]->is_variadic) {
            variadic_fns_.insert(hir->functions[i]->name);
        }
    }

    mod->fn_count = hir->fn_count;
    mod->functions = ctx_.arena.makeArray<LIRFunction>(hir->fn_count);

    for (uint32_t i = 0; i < hir->fn_count; ++i) {
        mod->functions[i] = buildFunction(hir->functions[i]);
    }

    // Add user-declared globals from HIR
    for (uint32_t i = 0; i < hir->global_count; ++i) {
        auto* hgd = hir->globals[i];
        GlobalData gd{};
        gd.kind = GlobalData::Variable;
        gd.index = static_cast<uint32_t>(globals_.size());
        gd.label = hgd->name;
        gd.variable.is_mutable = hgd->is_mutable;
        gd.variable.section_name = hgd->section_name;
        gd.variable.array_values = nullptr;
        gd.variable.array_count = 0;
        gd.variable.init_bytes = nullptr;
        gd.variable.init_byte_count = 0;
        // Evaluate constant initializer
        int64_t init_val = 0;
        if (hgd->init) {
            if (hgd->init->kind == HIRExpr::Kind::ArrayLit) {
                // Array literal: extract each element as a constant
                auto* arr = static_cast<const HIRArrayLitExpr*>(hgd->init);
                uint32_t n = arr->element_count;
                gd.variable.array_count = n;
                gd.variable.array_values = ctx_.arena.makeArray<int64_t>(n);
                TypeId elem_type = TypeTable::I64;
                if (hgd->type_id < ctx_.types.size()) {
                    const auto& ti = ctx_.types.get(hgd->type_id);
                    if (ti.kind == TypeKind::Array) elem_type = ti.array.element;
                }
                uint32_t elem_sz = ctx_.types.sizeOf(elem_type);
                gd.variable.size = static_cast<uint8_t>(elem_sz > 0 ? elem_sz : 8);
                for (uint32_t j = 0; j < n; ++j) {
                    int64_t v = 0;
                    if (arr->elements[j]->kind == HIRExpr::Kind::IntLit) {
                        v = static_cast<const HIRIntLitExpr*>(arr->elements[j])->value;
                    } else if (arr->elements[j]->kind == HIRExpr::Kind::BoolLit) {
                        v = static_cast<const HIRBoolLitExpr*>(arr->elements[j])->value ? 1 : 0;
                    }
                    gd.variable.array_values[j] = v;
                }
            } else if (hgd->init->kind == HIRExpr::Kind::FloatLit) {
                // Float literal: serialize to raw bytes
                auto* flit = static_cast<const HIRFloatLitExpr*>(hgd->init);
                bool is_f32 = (hgd->type_id == TypeTable::F32);
                uint32_t sz = is_f32 ? 4 : 8;
                gd.variable.size = static_cast<uint8_t>(sz);
                gd.variable.init_bytes = ctx_.arena.makeArray<uint8_t>(sz);
                gd.variable.init_byte_count = sz;
                if (is_f32) {
                    float f = static_cast<float>(flit->value);
                    std::memcpy(gd.variable.init_bytes, &f, 4);
                } else {
                    double d = flit->value;
                    std::memcpy(gd.variable.init_bytes, &d, 8);
                }
            } else if (hgd->init->kind == HIRExpr::Kind::StructLit) {
                // Struct literal: serialize to raw bytes using type layout
                auto* slit = static_cast<const HIRStructLitExpr*>(hgd->init);
                uint32_t total_sz = ctx_.types.sizeOf(hgd->type_id);
                if (total_sz > 0) {
                    gd.variable.size = static_cast<uint8_t>(total_sz > 255 ? 255 : total_sz);
                    gd.variable.init_bytes = ctx_.arena.makeArray<uint8_t>(total_sz);
                    gd.variable.init_byte_count = total_sz;
                    std::memset(gd.variable.init_bytes, 0, total_sz);
                    // Fill in each field
                    const auto& ti = ctx_.types.get(hgd->type_id);
                    if (ti.kind == TypeKind::Struct) {
                        for (uint32_t j = 0; j < slit->field_count; ++j) {
                            // Find field info by name
                            for (uint32_t fi = 0; fi < ti.struct_.field_count; ++fi) {
                                if (ti.struct_.fields[fi].name == slit->fields[j].name) {
                                    int32_t off = ti.struct_.fields[fi].offset;
                                    if (off < 0) break;
                                    uint32_t field_sz = ctx_.types.sizeOf(ti.struct_.fields[fi].type);
                                    auto* fexpr = slit->fields[j].value;
                                    if (fexpr->kind == HIRExpr::Kind::IntLit) {
                                        int64_t v = static_cast<const HIRIntLitExpr*>(fexpr)->value;
                                        if (ti.struct_.fields[fi].bit_width > 0) {
                                            uint32_t bw = ti.struct_.fields[fi].bit_width;
                                            uint32_t bo = ti.struct_.fields[fi].bit_offset;
                                            uint64_t mask = (1ULL << bw) - 1;
                                            uint64_t shifted = (static_cast<uint64_t>(v) & mask) << bo;
                                            uint64_t existing = 0;
                                            std::memcpy(&existing, gd.variable.init_bytes + off,
                                                       std::min(field_sz, total_sz - static_cast<uint32_t>(off)));
                                            existing |= shifted;
                                            std::memcpy(gd.variable.init_bytes + off, &existing,
                                                       std::min(field_sz, total_sz - static_cast<uint32_t>(off)));
                                        } else {
                                            std::memcpy(gd.variable.init_bytes + off, &v,
                                                       std::min(field_sz, 8u));
                                        }
                                    } else if (fexpr->kind == HIRExpr::Kind::BoolLit) {
                                        uint8_t b = static_cast<const HIRBoolLitExpr*>(fexpr)->value ? 1 : 0;
                                        gd.variable.init_bytes[off] = b;
                                    } else if (fexpr->kind == HIRExpr::Kind::FloatLit) {
                                        auto* fl = static_cast<const HIRFloatLitExpr*>(fexpr);
                                        bool f32 = (ti.struct_.fields[fi].type == TypeTable::F32);
                                        if (f32) {
                                            float f = static_cast<float>(fl->value);
                                            std::memcpy(gd.variable.init_bytes + off, &f, 4);
                                        } else {
                                            double d = fl->value;
                                            std::memcpy(gd.variable.init_bytes + off, &d, 8);
                                        }
                                    }
                                    break;
                                }
                            }
                        }
                    }
                } else {
                    gd.variable.size = 8;
                }
            } else {
                // Scalar integer/bool initializer
                if (hgd->init->kind == HIRExpr::Kind::IntLit) {
                    init_val = static_cast<const HIRIntLitExpr*>(hgd->init)->value;
                } else if (hgd->init->kind == HIRExpr::Kind::BoolLit) {
                    init_val = static_cast<const HIRBoolLitExpr*>(hgd->init)->value ? 1 : 0;
                }
                uint32_t sz = ctx_.types.sizeOf(hgd->type_id);
                gd.variable.size = static_cast<uint8_t>(sz > 0 ? sz : 8);
            }
        } else {
            // No initializer — determine size from type
            uint32_t sz = ctx_.types.sizeOf(hgd->type_id);
            gd.variable.size = static_cast<uint8_t>(sz > 0 ? sz : 8);
        }
        gd.variable.init_value = init_val;
        // Register label → GlobalData index for use in LoadGlobal
        global_label_map_[hgd->name] = gd.index;
        globals_.push_back(gd);
    }

    // Add vtable globals (arrays of function label references in .rodata)
    for (uint32_t i = 0; i < hir->vtable_count; ++i) {
        auto& vt = hir->vtables[i];
        GlobalData gd{};
        gd.kind = GlobalData::VTable;
        gd.index = static_cast<uint32_t>(globals_.size());
        gd.label = vt.label;
        gd.vtable.method_count = vt.method_count;
        gd.vtable.fn_labels = ctx_.arena.makeArray<std::string_view>(vt.method_count);
        for (uint32_t j = 0; j < vt.method_count; ++j) {
            gd.vtable.fn_labels[j] = vt.fn_labels[j];
        }
        global_label_map_[vt.label] = gd.index;
        globals_.push_back(gd);
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
    lir_fn.is_pub = fn->is_pub;
    lir_fn.is_extern = fn->is_extern;
    lir_fn.is_variadic = fn->is_variadic;
    lir_fn.is_weak = fn->is_weak;
    lir_fn.section_name = fn->section_name;
    lir_fn.link_name = fn->link_name;

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

    // Check for global variables
    auto gv_it = global_label_map_.find(expr->name);
    if (gv_it != global_label_map_.end()) {
        VReg r = freshVReg();
        LIRInstr i{};
        i.op = LIROp::LoadGlobal;
        i.result = r;
        // For vtable labels, use an array type so the backend emits LeaGlobal (address)
        if (vtable_labels_.count(expr->name)) {
            i.type = ctx_.types.makeArrayType(TypeTable::U64, 1);
        } else {
            i.type = expr->type;
        }
        i.load_global.label = expr->name;
        i.loc = expr->loc;
        emit(i);
        return r;
    }

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
        case HIRBinOp::AddWrap: lir_op = LIROp::AddWrap; break;
        case HIRBinOp::SubWrap: lir_op = LIROp::SubWrap; break;
        case HIRBinOp::MulWrap: lir_op = LIROp::MulWrap; break;
        case HIRBinOp::AddSat:  lir_op = LIROp::AddSat; break;
        case HIRBinOp::SubSat:  lir_op = LIROp::SubSat; break;
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

    // volatile_read(ptr) -> value (also matches monomorphized volatile_read_T)
    if ((callee == "volatile_read" || callee.starts_with("volatile_read_")) && expr->arg_count == 1) {
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

    // volatile_write(ptr, value) (also matches monomorphized volatile_write_T)
    if ((callee == "volatile_write" || callee.starts_with("volatile_write_")) && expr->arg_count == 2) {
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

    // Bit manipulation intrinsics: clz(x), ctz(x), popcnt(x), bswap(x)
    if ((callee == "clz" || callee == "ctz" || callee == "popcnt" || callee == "bswap")
        && expr->arg_count == 1) {
        VReg operand = lowerExpr(expr->args[0]);
        VReg r = freshVReg();
        LIRInstr i{};
        if (callee == "clz")       i.op = LIROp::Clz;
        else if (callee == "ctz")  i.op = LIROp::Ctz;
        else if (callee == "popcnt") i.op = LIROp::Popcnt;
        else                       i.op = LIROp::Bswap;
        i.result = r;
        i.type = expr->type;
        i.unary.operand = operand;
        i.loc = expr->loc;
        emit(i);
        return r;
    }

    // Port I/O intrinsics: inb(port), inw(port), inl(port)
    if ((callee == "inb" || callee == "inw" || callee == "inl") && expr->arg_count == 1) {
        VReg port = lowerExpr(expr->args[0]);
        VReg r = freshVReg();
        LIRInstr i{};
        i.op = LIROp::PortIn;
        i.result = r;
        i.type = expr->type;
        i.port_in.port = port;
        i.loc = expr->loc;
        emit(i);
        return r;
    }

    // Port I/O intrinsics: outb(port, val), outw(port, val), outl(port, val)
    if ((callee == "outb" || callee == "outw" || callee == "outl") && expr->arg_count == 2) {
        VReg port = lowerExpr(expr->args[0]);
        VReg val = lowerExpr(expr->args[1]);
        LIRInstr i{};
        i.op = LIROp::PortOut;
        i.result = INVALID_VREG;
        i.type = TypeTable::Unit;
        i.port_out.port = port;
        i.port_out.value = val;
        i.loc = expr->loc;
        emit(i);
        return INVALID_VREG;
    }

    // unreachable() → ud2 (undefined instruction trap)
    if (callee == "unreachable" && expr->arg_count == 0) {
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = INVALID_VREG;
        i.type = TypeTable::Never;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(1);
        auto* lens = ctx_.arena.makeArray<uint32_t>(1);
        lines[0] = "ud2"; lens[0] = 3;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 1;
        i.inline_asm.outputs = nullptr;
        i.inline_asm.output_count = 0;
        i.inline_asm.inputs = nullptr;
        i.inline_asm.input_count = 0;
        i.inline_asm.clobbers = nullptr;
        i.inline_asm.clobber_count = 0;
        emit(i);
        return INVALID_VREG;
    }

    // panic() → ud2 (traps the CPU; message arg is for future panic handler linkage)
    if (callee == "panic" && expr->arg_count <= 1) {
        // Lower the message arg (if any) for future use, but don't use it yet
        if (expr->arg_count == 1) {
            lowerExpr(expr->args[0]);
        }
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = INVALID_VREG;
        i.type = TypeTable::Never;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(1);
        auto* lens = ctx_.arena.makeArray<uint32_t>(1);
        lines[0] = "ud2"; lens[0] = 3;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 1;
        i.inline_asm.outputs = nullptr;
        i.inline_asm.output_count = 0;
        i.inline_asm.inputs = nullptr;
        i.inline_asm.input_count = 0;
        i.inline_asm.clobbers = nullptr;
        i.inline_asm.clobber_count = 0;
        emit(i);
        return INVALID_VREG;
    }

    // CPU control intrinsics (expand as inline asm)
    if (callee == "rdtsc" && expr->arg_count == 0) {
        // rdtsc → result in EDX:EAX, combine to 64-bit
        VReg r = freshVReg();
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = r;
        i.type = expr->type;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(3);
        auto* lens = ctx_.arena.makeArray<uint32_t>(3);
        lines[0] = "rdtsc"; lens[0] = 5;
        lines[1] = "shl rdx, 32"; lens[1] = 11;
        lines[2] = "or rax, rdx"; lens[2] = 11;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 3;
        auto* outs = ctx_.arena.makeArray<LIRAsmOperand>(1);
        outs[0].constraint = "=a";
        outs[0].vreg = r;
        i.inline_asm.outputs = outs;
        i.inline_asm.output_count = 1;
        i.inline_asm.inputs = nullptr;
        i.inline_asm.input_count = 0;
        auto* clobs = ctx_.arena.makeArray<std::string_view>(1);
        clobs[0] = "rdx";
        i.inline_asm.clobbers = clobs;
        i.inline_asm.clobber_count = 1;
        emit(i);
        return r;
    }

    if (callee == "rdmsr" && expr->arg_count == 1) {
        // rdmsr: ecx = msr_num, result in EDX:EAX
        VReg msr = lowerExpr(expr->args[0]);
        VReg r = freshVReg();
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = r;
        i.type = expr->type;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(3);
        auto* lens = ctx_.arena.makeArray<uint32_t>(3);
        lines[0] = "rdmsr"; lens[0] = 5;
        lines[1] = "shl rdx, 32"; lens[1] = 11;
        lines[2] = "or rax, rdx"; lens[2] = 11;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 3;
        auto* outs = ctx_.arena.makeArray<LIRAsmOperand>(1);
        outs[0].constraint = "=a";
        outs[0].vreg = r;
        i.inline_asm.outputs = outs;
        i.inline_asm.output_count = 1;
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(1);
        ins[0].constraint = "c";
        ins[0].vreg = msr;
        i.inline_asm.inputs = ins;
        i.inline_asm.input_count = 1;
        auto* clobs = ctx_.arena.makeArray<std::string_view>(1);
        clobs[0] = "rdx";
        i.inline_asm.clobbers = clobs;
        i.inline_asm.clobber_count = 1;
        emit(i);
        return r;
    }

    if (callee == "wrmsr" && expr->arg_count == 2) {
        // wrmsr: ecx = msr_num, edx:eax = value
        VReg msr = lowerExpr(expr->args[0]);
        VReg value = lowerExpr(expr->args[1]);
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = INVALID_VREG;
        i.type = TypeTable::Unit;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(4);
        auto* lens = ctx_.arena.makeArray<uint32_t>(4);
        lines[0] = "mov rax, $1"; lens[0] = 11;
        lines[1] = "mov rdx, $1"; lens[1] = 11;
        lines[2] = "shr rdx, 32"; lens[2] = 11;
        lines[3] = "wrmsr"; lens[3] = 5;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 4;
        i.inline_asm.outputs = nullptr;
        i.inline_asm.output_count = 0;
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(2);
        ins[0].constraint = "c";
        ins[0].vreg = msr;
        ins[1].constraint = "r";
        ins[1].vreg = value;
        i.inline_asm.inputs = ins;
        i.inline_asm.input_count = 2;
        auto* clobs = ctx_.arena.makeArray<std::string_view>(2);
        clobs[0] = "rax";
        clobs[1] = "rdx";
        i.inline_asm.clobbers = clobs;
        i.inline_asm.clobber_count = 2;
        emit(i);
        return INVALID_VREG;
    }

    // cpuid_eax/ebx/ecx/edx(leaf) — returns the specified register after cpuid
    if (expr->arg_count == 1 &&
        (callee == "cpuid_eax" || callee == "cpuid_ebx" ||
         callee == "cpuid_ecx" || callee == "cpuid_edx")) {
        VReg leaf = lowerExpr(expr->args[0]);
        VReg r = freshVReg();
        const char* out_constraint = "=a";
        if (callee == "cpuid_ebx") out_constraint = "=b";
        else if (callee == "cpuid_ecx") out_constraint = "=c";
        else if (callee == "cpuid_edx") out_constraint = "=d";

        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = r;
        i.type = expr->type;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(1);
        auto* lens = ctx_.arena.makeArray<uint32_t>(1);
        lines[0] = "cpuid"; lens[0] = 5;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 1;
        auto* outs = ctx_.arena.makeArray<LIRAsmOperand>(1);
        outs[0].constraint = out_constraint;
        outs[0].vreg = r;
        i.inline_asm.outputs = outs;
        i.inline_asm.output_count = 1;
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(1);
        ins[0].constraint = "a";
        ins[0].vreg = leaf;
        i.inline_asm.inputs = ins;
        i.inline_asm.input_count = 1;
        // Clobber the registers we don't output
        auto* clobs = ctx_.arena.makeArray<std::string_view>(3);
        uint32_t clob_count = 0;
        if (callee != "cpuid_eax") clobs[clob_count++] = "rax";
        if (callee != "cpuid_ebx") clobs[clob_count++] = "rbx";
        if (callee != "cpuid_ecx") clobs[clob_count++] = "rcx";
        if (callee != "cpuid_edx") clobs[clob_count++] = "rdx";
        i.inline_asm.clobbers = clobs;
        i.inline_asm.clobber_count = clob_count;
        emit(i);
        return r;
    }

    // cpuid_sub(leaf, subleaf) — cpuid with subleaf in ecx, returns eax
    if (callee == "cpuid_sub" && expr->arg_count == 2) {
        VReg leaf = lowerExpr(expr->args[0]);
        VReg subleaf = lowerExpr(expr->args[1]);
        VReg r = freshVReg();
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = r;
        i.type = expr->type;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(1);
        auto* lens = ctx_.arena.makeArray<uint32_t>(1);
        lines[0] = "cpuid"; lens[0] = 5;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 1;
        auto* outs = ctx_.arena.makeArray<LIRAsmOperand>(1);
        outs[0].constraint = "=a";
        outs[0].vreg = r;
        i.inline_asm.outputs = outs;
        i.inline_asm.output_count = 1;
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(2);
        ins[0].constraint = "a";
        ins[0].vreg = leaf;
        ins[1].constraint = "c";
        ins[1].vreg = subleaf;
        i.inline_asm.inputs = ins;
        i.inline_asm.input_count = 2;
        auto* clobs = ctx_.arena.makeArray<std::string_view>(2);
        clobs[0] = "rbx";
        clobs[1] = "rdx";
        i.inline_asm.clobbers = clobs;
        i.inline_asm.clobber_count = 2;
        emit(i);
        return r;
    }

    // Simple no-arg, no-return CPU control intrinsics
    if (expr->arg_count == 0 && expr->type == TypeTable::Unit) {
        const char* asm_str = nullptr;
        uint32_t asm_len = 0;
        if (callee == "cli")      { asm_str = "cli";      asm_len = 3; }
        if (callee == "sti")      { asm_str = "sti";      asm_len = 3; }
        if (callee == "hlt")      { asm_str = "hlt";      asm_len = 3; }
        if (callee == "pause")    { asm_str = "pause";    asm_len = 5; }
        if (callee == "swapgs")   { asm_str = "swapgs";   asm_len = 6; }
        if (callee == "wbinvd")   { asm_str = "wbinvd";   asm_len = 6; }
        if (callee == "stac")     { asm_str = "stac";     asm_len = 4; }
        if (callee == "clac")     { asm_str = "clac";     asm_len = 4; }
        if (callee == "sysretq")  { asm_str = "sysretq";  asm_len = 7; }
        if (callee == "sysenter") { asm_str = "sysenter"; asm_len = 8; }
        if (callee == "sysexit")  { asm_str = "sysexit";  asm_len = 7; }
        if (asm_str) {
            LIRInstr i{};
            i.op = LIROp::InlineAsm;
            i.result = INVALID_VREG;
            i.type = TypeTable::Unit;
            i.loc = expr->loc;
            auto* lines = ctx_.arena.makeArray<const char*>(1);
            auto* lens = ctx_.arena.makeArray<uint32_t>(1);
            lines[0] = asm_str; lens[0] = asm_len;
            i.inline_asm.lines = lines;
            i.inline_asm.line_lengths = lens;
            i.inline_asm.line_count = 1;
            i.inline_asm.outputs = nullptr;
            i.inline_asm.output_count = 0;
            i.inline_asm.inputs = nullptr;
            i.inline_asm.input_count = 0;
            i.inline_asm.clobbers = nullptr;
            i.inline_asm.clobber_count = 0;
            emit(i);
            return INVALID_VREG;
        }
    }

    // Read control register intrinsics (no arg → u64)
    if (expr->arg_count == 0) {
        const char* cr_name = nullptr;
        uint32_t cr_len = 0;
        if (callee == "read_cr0") { cr_name = "mov rax, cr0"; cr_len = 12; }
        if (callee == "read_cr2") { cr_name = "mov rax, cr2"; cr_len = 12; }
        if (callee == "read_cr3") { cr_name = "mov rax, cr3"; cr_len = 12; }
        if (callee == "read_cr4") { cr_name = "mov rax, cr4"; cr_len = 12; }
        if (cr_name) {
            VReg r = freshVReg();
            LIRInstr i{};
            i.op = LIROp::InlineAsm;
            i.result = r;
            i.type = expr->type;
            i.loc = expr->loc;
            auto* lines = ctx_.arena.makeArray<const char*>(1);
            auto* lens = ctx_.arena.makeArray<uint32_t>(1);
            lines[0] = cr_name; lens[0] = cr_len;
            i.inline_asm.lines = lines;
            i.inline_asm.line_lengths = lens;
            i.inline_asm.line_count = 1;
            auto* outs = ctx_.arena.makeArray<LIRAsmOperand>(1);
            outs[0].constraint = "=a";
            outs[0].vreg = r;
            i.inline_asm.outputs = outs;
            i.inline_asm.output_count = 1;
            i.inline_asm.inputs = nullptr;
            i.inline_asm.input_count = 0;
            i.inline_asm.clobbers = nullptr;
            i.inline_asm.clobber_count = 0;
            emit(i);
            return r;
        }
    }

    // Write control register intrinsics (u64 → Unit)
    if (expr->arg_count == 1 && expr->type == TypeTable::Unit) {
        const char* cr_asm = nullptr;
        uint32_t cr_asm_len = 0;
        if (callee == "write_cr0") { cr_asm = "mov cr0, $0"; cr_asm_len = 11; }
        if (callee == "write_cr3") { cr_asm = "mov cr3, $0"; cr_asm_len = 11; }
        if (callee == "write_cr4") { cr_asm = "mov cr4, $0"; cr_asm_len = 11; }
        if (cr_asm) {
            VReg val = lowerExpr(expr->args[0]);
            LIRInstr i{};
            i.op = LIROp::InlineAsm;
            i.result = INVALID_VREG;
            i.type = TypeTable::Unit;
            i.loc = expr->loc;
            auto* lines = ctx_.arena.makeArray<const char*>(1);
            auto* lens = ctx_.arena.makeArray<uint32_t>(1);
            lines[0] = cr_asm; lens[0] = cr_asm_len;
            i.inline_asm.lines = lines;
            i.inline_asm.line_lengths = lens;
            i.inline_asm.line_count = 1;
            i.inline_asm.outputs = nullptr;
            i.inline_asm.output_count = 0;
            auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(1);
            ins[0].constraint = "r";
            ins[0].vreg = val;
            i.inline_asm.inputs = ins;
            i.inline_asm.input_count = 1;
            i.inline_asm.clobbers = nullptr;
            i.inline_asm.clobber_count = 0;
            emit(i);
            return INVALID_VREG;
        }
    }

    // invlpg(addr) — TLB invalidation for single page
    if (callee == "invlpg" && expr->arg_count == 1) {
        VReg addr = lowerExpr(expr->args[0]);
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = INVALID_VREG;
        i.type = TypeTable::Unit;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(1);
        auto* lens = ctx_.arena.makeArray<uint32_t>(1);
        lines[0] = "invlpg [$0]"; lens[0] = 11;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 1;
        i.inline_asm.outputs = nullptr;
        i.inline_asm.output_count = 0;
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(1);
        ins[0].constraint = "r";
        ins[0].vreg = addr;
        i.inline_asm.inputs = ins;
        i.inline_asm.input_count = 1;
        i.inline_asm.clobbers = nullptr;
        i.inline_asm.clobber_count = 0;
        emit(i);
        return INVALID_VREG;
    }

    // lgdt/lidt — load GDT/IDT descriptor pointer
    if ((callee == "lgdt" || callee == "lidt") && expr->arg_count == 1) {
        VReg ptr = lowerExpr(expr->args[0]);
        const char* inst = (callee == "lgdt") ? "lgdt [$0]" : "lidt [$0]";
        uint32_t inst_len = 9;
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = INVALID_VREG;
        i.type = TypeTable::Unit;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(1);
        auto* lens = ctx_.arena.makeArray<uint32_t>(1);
        lines[0] = inst; lens[0] = inst_len;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 1;
        i.inline_asm.outputs = nullptr;
        i.inline_asm.output_count = 0;
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(1);
        ins[0].constraint = "r";
        ins[0].vreg = ptr;
        i.inline_asm.inputs = ins;
        i.inline_asm.input_count = 1;
        i.inline_asm.clobbers = nullptr;
        i.inline_asm.clobber_count = 0;
        emit(i);
        return INVALID_VREG;
    }

    // ltr — load task register (u16 selector)
    if (callee == "ltr" && expr->arg_count == 1) {
        VReg sel = lowerExpr(expr->args[0]);
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = INVALID_VREG;
        i.type = TypeTable::Unit;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(1);
        auto* lens = ctx_.arena.makeArray<uint32_t>(1);
        lines[0] = "ltr $0w"; lens[0] = 7;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 1;
        i.inline_asm.outputs = nullptr;
        i.inline_asm.output_count = 0;
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(1);
        ins[0].constraint = "r";
        ins[0].vreg = sel;
        i.inline_asm.inputs = ins;
        i.inline_asm.input_count = 1;
        i.inline_asm.clobbers = nullptr;
        i.inline_asm.clobber_count = 0;
        emit(i);
        return INVALID_VREG;
    }

    // fxsave(ptr) / fxrstor(ptr) — save/restore FPU+SSE state (512 bytes)
    if ((callee == "fxsave" || callee == "fxrstor") && expr->arg_count == 1) {
        VReg ptr = lowerExpr(expr->args[0]);
        const char* inst = (callee == "fxsave") ? "fxsave [$0]" : "fxrstor [$0]";
        uint32_t inst_len = (callee == "fxsave") ? 11u : 12u;
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = INVALID_VREG;
        i.type = TypeTable::Unit;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(1);
        auto* lens = ctx_.arena.makeArray<uint32_t>(1);
        lines[0] = inst; lens[0] = inst_len;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 1;
        i.inline_asm.outputs = nullptr;
        i.inline_asm.output_count = 0;
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(1);
        ins[0].constraint = "r";
        ins[0].vreg = ptr;
        i.inline_asm.inputs = ins;
        i.inline_asm.input_count = 1;
        i.inline_asm.clobbers = nullptr;
        i.inline_asm.clobber_count = 0;
        emit(i);
        return INVALID_VREG;
    }

    // xsave(ptr, mask) / xrstor(ptr, mask) — extended save/restore
    // mask is split into EDX:EAX (high:low 32 bits)
    if ((callee == "xsave" || callee == "xrstor") && expr->arg_count == 2) {
        VReg ptr = lowerExpr(expr->args[0]);
        VReg mask = lowerExpr(expr->args[1]);
        const char* inst = (callee == "xsave") ? "xsave [$0]" : "xrstor [$0]";
        uint32_t inst_len = (callee == "xsave") ? 10u : 11u;
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = INVALID_VREG;
        i.type = TypeTable::Unit;
        i.loc = expr->loc;
        // Two lines: split mask into edx:eax, then xsave/xrstor
        auto* lines = ctx_.arena.makeArray<const char*>(4);
        auto* lens = ctx_.arena.makeArray<uint32_t>(4);
        lines[0] = "mov rax, $1"; lens[0] = 11;
        lines[1] = "mov rdx, rax"; lens[1] = 12;
        lines[2] = "shr rdx, 32"; lens[2] = 11;
        lines[3] = inst; lens[3] = inst_len;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 4;
        i.inline_asm.outputs = nullptr;
        i.inline_asm.output_count = 0;
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(2);
        ins[0].constraint = "r";
        ins[0].vreg = ptr;
        ins[1].constraint = "r";
        ins[1].vreg = mask;
        i.inline_asm.inputs = ins;
        i.inline_asm.input_count = 2;
        auto* clobs = ctx_.arena.makeArray<std::string_view>(2);
        clobs[0] = "rax";
        clobs[1] = "rdx";
        i.inline_asm.clobbers = clobs;
        i.inline_asm.clobber_count = 2;
        emit(i);
        return INVALID_VREG;
    }

    // Debug register read intrinsics (no arg → u64)
    if (expr->arg_count == 0) {
        const char* dr_asm = nullptr;
        uint32_t dr_len = 0;
        if (callee == "read_dr0") { dr_asm = "mov rax, dr0"; dr_len = 12; }
        if (callee == "read_dr1") { dr_asm = "mov rax, dr1"; dr_len = 12; }
        if (callee == "read_dr2") { dr_asm = "mov rax, dr2"; dr_len = 12; }
        if (callee == "read_dr3") { dr_asm = "mov rax, dr3"; dr_len = 12; }
        if (callee == "read_dr6") { dr_asm = "mov rax, dr6"; dr_len = 12; }
        if (callee == "read_dr7") { dr_asm = "mov rax, dr7"; dr_len = 12; }
        if (dr_asm) {
            VReg r = freshVReg();
            LIRInstr i{};
            i.op = LIROp::InlineAsm;
            i.result = r;
            i.type = expr->type;
            i.loc = expr->loc;
            auto* lines = ctx_.arena.makeArray<const char*>(1);
            auto* lens = ctx_.arena.makeArray<uint32_t>(1);
            lines[0] = dr_asm; lens[0] = dr_len;
            i.inline_asm.lines = lines;
            i.inline_asm.line_lengths = lens;
            i.inline_asm.line_count = 1;
            auto* outs = ctx_.arena.makeArray<LIRAsmOperand>(1);
            outs[0].constraint = "=a";
            outs[0].vreg = r;
            i.inline_asm.outputs = outs;
            i.inline_asm.output_count = 1;
            i.inline_asm.inputs = nullptr;
            i.inline_asm.input_count = 0;
            i.inline_asm.clobbers = nullptr;
            i.inline_asm.clobber_count = 0;
            emit(i);
            return r;
        }
    }

    // Debug register write intrinsics (u64 → Unit)
    if (expr->arg_count == 1 && expr->type == TypeTable::Unit) {
        const char* dr_asm = nullptr;
        uint32_t dr_len = 0;
        if (callee == "write_dr0") { dr_asm = "mov dr0, $0"; dr_len = 11; }
        if (callee == "write_dr1") { dr_asm = "mov dr1, $0"; dr_len = 11; }
        if (callee == "write_dr2") { dr_asm = "mov dr2, $0"; dr_len = 11; }
        if (callee == "write_dr3") { dr_asm = "mov dr3, $0"; dr_len = 11; }
        if (callee == "write_dr6") { dr_asm = "mov dr6, $0"; dr_len = 11; }
        if (callee == "write_dr7") { dr_asm = "mov dr7, $0"; dr_len = 11; }
        if (dr_asm) {
            VReg val = lowerExpr(expr->args[0]);
            LIRInstr i{};
            i.op = LIROp::InlineAsm;
            i.result = INVALID_VREG;
            i.type = TypeTable::Unit;
            i.loc = expr->loc;
            auto* lines = ctx_.arena.makeArray<const char*>(1);
            auto* lens = ctx_.arena.makeArray<uint32_t>(1);
            lines[0] = dr_asm; lens[0] = dr_len;
            i.inline_asm.lines = lines;
            i.inline_asm.line_lengths = lens;
            i.inline_asm.line_count = 1;
            i.inline_asm.outputs = nullptr;
            i.inline_asm.output_count = 0;
            auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(1);
            ins[0].constraint = "r";
            ins[0].vreg = val;
            i.inline_asm.inputs = ins;
            i.inline_asm.input_count = 1;
            i.inline_asm.clobbers = nullptr;
            i.inline_asm.clobber_count = 0;
            emit(i);
            return INVALID_VREG;
        }
    }

    // FS/GS base register intrinsics
    if (expr->arg_count == 0) {
        const char* fsgs_asm = nullptr;
        uint32_t fsgs_len = 0;
        if (callee == "rdfsbase") { fsgs_asm = "rdfsbase rax"; fsgs_len = 13; }
        if (callee == "rdgsbase") { fsgs_asm = "rdgsbase rax"; fsgs_len = 13; }
        if (fsgs_asm) {
            VReg r = freshVReg();
            LIRInstr i{};
            i.op = LIROp::InlineAsm;
            i.result = r;
            i.type = expr->type;
            i.loc = expr->loc;
            auto* lines = ctx_.arena.makeArray<const char*>(1);
            auto* lens = ctx_.arena.makeArray<uint32_t>(1);
            lines[0] = fsgs_asm; lens[0] = fsgs_len;
            i.inline_asm.lines = lines;
            i.inline_asm.line_lengths = lens;
            i.inline_asm.line_count = 1;
            auto* outs = ctx_.arena.makeArray<LIRAsmOperand>(1);
            outs[0].constraint = "=a";
            outs[0].vreg = r;
            i.inline_asm.outputs = outs;
            i.inline_asm.output_count = 1;
            i.inline_asm.inputs = nullptr;
            i.inline_asm.input_count = 0;
            i.inline_asm.clobbers = nullptr;
            i.inline_asm.clobber_count = 0;
            emit(i);
            return r;
        }
    }

    if (expr->arg_count == 1 && expr->type == TypeTable::Unit) {
        const char* fsgs_asm = nullptr;
        uint32_t fsgs_len = 0;
        if (callee == "wrfsbase") { fsgs_asm = "wrfsbase $0"; fsgs_len = 11; }
        if (callee == "wrgsbase") { fsgs_asm = "wrgsbase $0"; fsgs_len = 11; }
        if (fsgs_asm) {
            VReg val = lowerExpr(expr->args[0]);
            LIRInstr i{};
            i.op = LIROp::InlineAsm;
            i.result = INVALID_VREG;
            i.type = TypeTable::Unit;
            i.loc = expr->loc;
            auto* lines = ctx_.arena.makeArray<const char*>(1);
            auto* lens = ctx_.arena.makeArray<uint32_t>(1);
            lines[0] = fsgs_asm; lens[0] = fsgs_len;
            i.inline_asm.lines = lines;
            i.inline_asm.line_lengths = lens;
            i.inline_asm.line_count = 1;
            i.inline_asm.outputs = nullptr;
            i.inline_asm.output_count = 0;
            auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(1);
            ins[0].constraint = "r";
            ins[0].vreg = val;
            i.inline_asm.inputs = ins;
            i.inline_asm.input_count = 1;
            i.inline_asm.clobbers = nullptr;
            i.inline_asm.clobber_count = 0;
            emit(i);
            return INVALID_VREG;
        }
    }

    // Cache management intrinsics (ptr → Unit)
    if (expr->arg_count == 1 && expr->type == TypeTable::Unit) {
        const char* cache_asm = nullptr;
        uint32_t cache_len = 0;
        if (callee == "clflush")    { cache_asm = "clflush [$0]";    cache_len = 12; }
        if (callee == "clflushopt") { cache_asm = "clflushopt [$0]"; cache_len = 15; }
        if (callee == "clwb")       { cache_asm = "clwb [$0]";       cache_len = 9; }
        if (cache_asm) {
            VReg addr = lowerExpr(expr->args[0]);
            LIRInstr i{};
            i.op = LIROp::InlineAsm;
            i.result = INVALID_VREG;
            i.type = TypeTable::Unit;
            i.loc = expr->loc;
            auto* lines = ctx_.arena.makeArray<const char*>(1);
            auto* lens = ctx_.arena.makeArray<uint32_t>(1);
            lines[0] = cache_asm; lens[0] = cache_len;
            i.inline_asm.lines = lines;
            i.inline_asm.line_lengths = lens;
            i.inline_asm.line_count = 1;
            i.inline_asm.outputs = nullptr;
            i.inline_asm.output_count = 0;
            auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(1);
            ins[0].constraint = "r";
            ins[0].vreg = addr;
            i.inline_asm.inputs = ins;
            i.inline_asm.input_count = 1;
            i.inline_asm.clobbers = nullptr;
            i.inline_asm.clobber_count = 0;
            emit(i);
            return INVALID_VREG;
        }
    }

    // Segment descriptor intrinsics
    // lldt(selector) — load local descriptor table register
    if (callee == "lldt" && expr->arg_count == 1) {
        VReg sel = lowerExpr(expr->args[0]);
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = INVALID_VREG;
        i.type = TypeTable::Unit;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(1);
        auto* lens = ctx_.arena.makeArray<uint32_t>(1);
        lines[0] = "lldt $0w"; lens[0] = 8;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 1;
        i.inline_asm.outputs = nullptr;
        i.inline_asm.output_count = 0;
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(1);
        ins[0].constraint = "r";
        ins[0].vreg = sel;
        i.inline_asm.inputs = ins;
        i.inline_asm.input_count = 1;
        i.inline_asm.clobbers = nullptr;
        i.inline_asm.clobber_count = 0;
        emit(i);
        return INVALID_VREG;
    }

    // sldt() → u64 — store local descriptor table register
    if (callee == "sldt" && expr->arg_count == 0) {
        VReg r = freshVReg();
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = r;
        i.type = expr->type;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(2);
        auto* lens = ctx_.arena.makeArray<uint32_t>(2);
        lines[0] = "xor eax, eax"; lens[0] = 12;
        lines[1] = "sldt ax"; lens[1] = 7;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 2;
        auto* outs = ctx_.arena.makeArray<LIRAsmOperand>(1);
        outs[0].constraint = "=a";
        outs[0].vreg = r;
        i.inline_asm.outputs = outs;
        i.inline_asm.output_count = 1;
        i.inline_asm.inputs = nullptr;
        i.inline_asm.input_count = 0;
        i.inline_asm.clobbers = nullptr;
        i.inline_asm.clobber_count = 0;
        emit(i);
        return r;
    }

    // lmsw(value) — load machine status word (bits 0-3 of CR0)
    if (callee == "lmsw" && expr->arg_count == 1) {
        VReg val = lowerExpr(expr->args[0]);
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = INVALID_VREG;
        i.type = TypeTable::Unit;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(1);
        auto* lens = ctx_.arena.makeArray<uint32_t>(1);
        lines[0] = "lmsw $0w"; lens[0] = 8;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 1;
        i.inline_asm.outputs = nullptr;
        i.inline_asm.output_count = 0;
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(1);
        ins[0].constraint = "r";
        ins[0].vreg = val;
        i.inline_asm.inputs = ins;
        i.inline_asm.input_count = 1;
        i.inline_asm.clobbers = nullptr;
        i.inline_asm.clobber_count = 0;
        emit(i);
        return INVALID_VREG;
    }

    // smsw() → u64 — store machine status word
    if (callee == "smsw" && expr->arg_count == 0) {
        VReg r = freshVReg();
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = r;
        i.type = expr->type;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(1);
        auto* lens = ctx_.arena.makeArray<uint32_t>(1);
        lines[0] = "smsw rax"; lens[0] = 8;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 1;
        auto* outs = ctx_.arena.makeArray<LIRAsmOperand>(1);
        outs[0].constraint = "=a";
        outs[0].vreg = r;
        i.inline_asm.outputs = outs;
        i.inline_asm.output_count = 1;
        i.inline_asm.inputs = nullptr;
        i.inline_asm.input_count = 0;
        i.inline_asm.clobbers = nullptr;
        i.inline_asm.clobber_count = 0;
        emit(i);
        return r;
    }

    // rdpmc(counter) → u64 — read performance monitoring counter
    if (callee == "rdpmc" && expr->arg_count == 1) {
        VReg cnt = lowerExpr(expr->args[0]);
        VReg r = freshVReg();
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = r;
        i.type = expr->type;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(3);
        auto* lens = ctx_.arena.makeArray<uint32_t>(3);
        lines[0] = "rdpmc"; lens[0] = 5;
        lines[1] = "shl rdx, 32"; lens[1] = 11;
        lines[2] = "or rax, rdx"; lens[2] = 11;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 3;
        auto* outs = ctx_.arena.makeArray<LIRAsmOperand>(1);
        outs[0].constraint = "=a";
        outs[0].vreg = r;
        i.inline_asm.outputs = outs;
        i.inline_asm.output_count = 1;
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(1);
        ins[0].constraint = "c";  // ecx = counter index
        ins[0].vreg = cnt;
        i.inline_asm.inputs = ins;
        i.inline_asm.input_count = 1;
        auto* clobs = ctx_.arena.makeArray<std::string_view>(1);
        clobs[0] = "rdx";
        i.inline_asm.clobbers = clobs;
        i.inline_asm.clobber_count = 1;
        emit(i);
        return r;
    }

    // memcpy(dst, src, count) — rep movsb
    if (callee == "memcpy" && expr->arg_count == 3) {
        VReg dst = lowerExpr(expr->args[0]);
        VReg src = lowerExpr(expr->args[1]);
        VReg cnt = lowerExpr(expr->args[2]);
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = INVALID_VREG;
        i.type = TypeTable::Unit;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(2);
        auto* lens = ctx_.arena.makeArray<uint32_t>(2);
        lines[0] = "cld"; lens[0] = 3;
        lines[1] = "rep movsb"; lens[1] = 9;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 2;
        i.inline_asm.outputs = nullptr;
        i.inline_asm.output_count = 0;
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(3);
        ins[0].constraint = "D"; ins[0].vreg = dst;  // rdi = dst
        ins[1].constraint = "S"; ins[1].vreg = src;  // rsi = src
        ins[2].constraint = "c"; ins[2].vreg = cnt;  // rcx = count
        i.inline_asm.inputs = ins;
        i.inline_asm.input_count = 3;
        auto* clobs = ctx_.arena.makeArray<std::string_view>(3);
        clobs[0] = "rdi"; clobs[1] = "rsi"; clobs[2] = "rcx";
        i.inline_asm.clobbers = clobs;
        i.inline_asm.clobber_count = 3;
        emit(i);
        return INVALID_VREG;
    }

    // memset(dst, byte, count) — rep stosb
    if (callee == "memset" && expr->arg_count == 3) {
        VReg dst = lowerExpr(expr->args[0]);
        VReg byte_val = lowerExpr(expr->args[1]);
        VReg cnt = lowerExpr(expr->args[2]);
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = INVALID_VREG;
        i.type = TypeTable::Unit;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(2);
        auto* lens = ctx_.arena.makeArray<uint32_t>(2);
        lines[0] = "cld"; lens[0] = 3;
        lines[1] = "rep stosb"; lens[1] = 9;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 2;
        i.inline_asm.outputs = nullptr;
        i.inline_asm.output_count = 0;
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(3);
        ins[0].constraint = "D"; ins[0].vreg = dst;       // rdi = dst
        ins[1].constraint = "a"; ins[1].vreg = byte_val;  // al = byte value
        ins[2].constraint = "c"; ins[2].vreg = cnt;        // rcx = count
        i.inline_asm.inputs = ins;
        i.inline_asm.input_count = 3;
        auto* clobs = ctx_.arena.makeArray<std::string_view>(3);
        clobs[0] = "rdi"; clobs[1] = "rax"; clobs[2] = "rcx";
        i.inline_asm.clobbers = clobs;
        i.inline_asm.clobber_count = 3;
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
    i.call.callee_module = expr->callee_module;
    i.call.args = args;
    i.call.arg_count = expr->arg_count;
    i.call.is_tail = expr->is_tail_call;
    i.call.is_variadic = variadic_fns_.count(expr->callee) > 0;
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
    i.fn_ref.fn_module = expr->fn_module;
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
                bool union_repr_c = (type_info.kind == TypeKind::Union &&
                                     type_info.union_.is_repr_c);

                if (!union_repr_c) {
                    // Tagged union: load tag from offset 0 and compare
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
                } else {
                    // Untagged union: no tag to check, always enter body
                    emitBranch(body_bb);
                }

                // Bind inner pattern (payload extraction) in body block
                if (pat->inner && pat->inner->kind == HIRPattern::Kind::Variable) {
                    auto* var_pat = static_cast<const HIRVariablePattern*>(pat->inner);
                    switchToBlock(body_bb);
                    // Untagged: payload at offset 0; tagged: payload at offset 8
                    uint32_t payload_offset = union_repr_c ? 0 : 8;

                    VReg payload_ptr = freshVReg();
                    LIRInstr fp2{};
                    fp2.op = LIROp::FieldPtr;
                    fp2.result = payload_ptr;
                    TypeId payload_type = type_info.union_.variants[tag].payload_type;
                    fp2.type = ctx_.types.makePtr(payload_type, false);
                    fp2.field_ptr.base = scrutinee;
                    fp2.field_ptr.offset = payload_offset;
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
    // Handle DynTrait fat pointer construction: { __data, __vtable }
    if (expr->type < ctx_.types.size() &&
        ctx_.types.get(expr->type).kind == TypeKind::DynTrait) {
        VReg base = freshVReg();
        LIRInstr alloc{};
        alloc.op = LIROp::StructAlloc;
        alloc.result = base;
        alloc.type = expr->type;
        alloc.struct_alloc.size = 16;
        alloc.struct_alloc.align = 8;
        alloc.loc = expr->loc;
        emit(alloc);
        for (uint32_t f = 0; f < expr->field_count; ++f) {
            VReg val = lowerExpr(expr->fields[f].value);
            uint32_t offset = (expr->fields[f].name == "__data") ? 0 : 8;
            VReg fp = freshVReg();
            LIRInstr fp_instr{};
            fp_instr.op = LIROp::FieldPtr;
            fp_instr.result = fp;
            fp_instr.type = ctx_.types.makePtr(TypeTable::U64, false);
            fp_instr.field_ptr.base = base;
            fp_instr.field_ptr.offset = offset;
            fp_instr.loc = expr->loc;
            emit(fp_instr);
            LIRInstr store{};
            store.op = LIROp::Store;
            store.type = TypeTable::U64;
            store.store.ptr = fp;
            store.store.value = val;
            store.loc = expr->loc;
            emit(store);
        }
        return base;
    }

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
    // Track which storage unit offsets have been initialized (for bitfield packing)
    std::unordered_set<uint32_t> initialized_offsets;

    for (uint32_t f = 0; f < expr->field_count; ++f) {
        VReg val = lowerExpr(expr->fields[f].value);
        uint32_t offset = structFieldOffset(expr->type, expr->fields[f].name);

        TypeId field_type = TypeTable::I64;
        const FieldInfo* field_info = nullptr;
        if (ti.kind == TypeKind::Struct) {
            for (uint32_t fi = 0; fi < ti.struct_.field_count; ++fi) {
                if (ti.struct_.fields[fi].name == expr->fields[f].name) {
                    field_type = ti.struct_.fields[fi].type;
                    field_info = &ti.struct_.fields[fi];
                    break;
                }
            }
            assert(field_info && "struct field not found — should be caught by HIRBuilder");
        }

        if (field_info && field_info->bit_width > 0) {
            // Bitfield initialization: pack into shared storage unit
            TypeId unit_type = field_info->type;

            VReg fp = freshVReg();
            LIRInstr fp_instr{};
            fp_instr.op = LIROp::FieldPtr;
            fp_instr.result = fp;
            fp_instr.type = ctx_.types.makePtr(unit_type, false);
            fp_instr.field_ptr.base = base;
            fp_instr.field_ptr.offset = offset;
            fp_instr.loc = expr->fields[f].loc;
            emit(fp_instr);

            // Shift value into position: val << bit_offset
            VReg shifted_val = val;
            if (field_info->bit_offset > 0) {
                VReg sh_amt = freshVReg();
                LIRInstr sha{};
                sha.op = LIROp::ConstInt;
                sha.result = sh_amt;
                sha.type = unit_type;
                sha.const_int.value = field_info->bit_offset;
                emit(sha);

                VReg sv = freshVReg();
                LIRInstr shl{};
                shl.op = LIROp::Shl;
                shl.result = sv;
                shl.type = unit_type;
                shl.bin.lhs = val;
                shl.bin.rhs = sh_amt;
                emit(shl);
                shifted_val = sv;
            }

            if (initialized_offsets.count(offset)) {
                // Not the first bitfield at this offset: load → OR → store
                VReg old_val = freshVReg();
                LIRInstr ld{};
                ld.op = LIROp::Load;
                ld.result = old_val;
                ld.type = unit_type;
                ld.load.ptr = fp;
                emit(ld);

                VReg combined = freshVReg();
                LIRInstr bo{};
                bo.op = LIROp::BOr;
                bo.result = combined;
                bo.type = unit_type;
                bo.bin.lhs = old_val;
                bo.bin.rhs = shifted_val;
                emit(bo);

                LIRInstr store{};
                store.op = LIROp::Store;
                store.result = INVALID_VREG;
                store.type = TypeTable::Unit;
                store.store.ptr = fp;
                store.store.value = combined;
                emit(store);
            } else {
                // First bitfield at this offset: direct store
                initialized_offsets.insert(offset);
                LIRInstr store{};
                store.op = LIROp::Store;
                store.result = INVALID_VREG;
                store.type = TypeTable::Unit;
                store.store.ptr = fp;
                store.store.value = shifted_val;
                emit(store);
            }
        } else {
            // Regular (non-bitfield) field: direct store
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
    }

    return base;
}

VReg LIRBuilder::lowerFieldAccess(const HIRFieldAccessExpr* expr) {
    VReg obj = lowerExpr(expr->object);

    // Handle DynTrait field access: __data at offset 0, __vtable at offset 8
    if (expr->object->type < ctx_.types.size() &&
        ctx_.types.get(expr->object->type).kind == TypeKind::DynTrait) {
        uint32_t offset = (expr->field_name == "__data") ? 0 : 8;
        VReg fp = freshVReg();
        LIRInstr fp_instr{};
        fp_instr.op = LIROp::FieldPtr;
        fp_instr.result = fp;
        fp_instr.type = ctx_.types.makePtr(TypeTable::U64, false);
        fp_instr.field_ptr.base = obj;
        fp_instr.field_ptr.offset = offset;
        fp_instr.loc = expr->loc;
        emit(fp_instr);
        VReg result = freshVReg();
        LIRInstr load{};
        load.op = LIROp::Load;
        load.result = result;
        load.type = expr->type;
        load.load.ptr = fp;
        load.loc = expr->loc;
        emit(load);
        return result;
    }

    uint32_t offset = structFieldOffset(expr->object->type, expr->field_name);

    // Check for bitfield access
    auto* fi = lookupField(ctx_.types, expr->object->type, expr->field_name);
    if (fi && fi->bit_width > 0) {
        // Bitfield read: load storage unit, shift right, mask
        TypeId unit_type = fi->type;  // storage type (e.g., u64)

        VReg fp = freshVReg();
        LIRInstr fp_instr{};
        fp_instr.op = LIROp::FieldPtr;
        fp_instr.result = fp;
        fp_instr.type = ctx_.types.makePtr(unit_type, false);
        fp_instr.field_ptr.base = obj;
        fp_instr.field_ptr.offset = offset;
        fp_instr.loc = expr->loc;
        emit(fp_instr);

        VReg unit_val = freshVReg();
        LIRInstr load{};
        load.op = LIROp::Load;
        load.result = unit_val;
        load.type = unit_type;
        load.load.ptr = fp;
        load.loc = expr->loc;
        emit(load);

        VReg result = unit_val;

        // Shift right by bit_offset
        if (fi->bit_offset > 0) {
            VReg shift_amt = freshVReg();
            LIRInstr shr_const{};
            shr_const.op = LIROp::ConstInt;
            shr_const.result = shift_amt;
            shr_const.type = unit_type;
            shr_const.const_int.value = fi->bit_offset;
            emit(shr_const);

            VReg shifted = freshVReg();
            LIRInstr shr{};
            shr.op = LIROp::Shr;
            shr.result = shifted;
            shr.type = unit_type;
            shr.bin.lhs = result;
            shr.bin.rhs = shift_amt;
            emit(shr);
            result = shifted;
        }

        // Mask to bit_width bits
        uint64_t mask = (fi->bit_width >= 64) ? ~0ULL : ((1ULL << fi->bit_width) - 1);
        VReg mask_val = freshVReg();
        LIRInstr mask_const{};
        mask_const.op = LIROp::ConstInt;
        mask_const.result = mask_val;
        mask_const.type = unit_type;
        mask_const.const_int.value = static_cast<int64_t>(mask);
        emit(mask_const);

        VReg masked = freshVReg();
        LIRInstr band{};
        band.op = LIROp::BAnd;
        band.result = masked;
        band.type = expr->type;
        band.bin.lhs = result;
        band.bin.rhs = mask_val;
        emit(band);
        return masked;
    }

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
    auto& ti = ctx_.types.get(expr->type);
    bool is_repr_c = (ti.kind == TypeKind::Union && ti.union_.is_repr_c);

    VReg base = freshVReg();
    LIRInstr alloc{};
    alloc.op = LIROp::StructAlloc;
    alloc.result = base;
    alloc.type = expr->type;
    alloc.struct_alloc.size = size;
    alloc.struct_alloc.align = align;
    alloc.loc = expr->loc;
    emit(alloc);

    if (!is_repr_c) {
        // Tagged union: write discriminant tag at offset 0
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
    }

    if (expr->payload) {
        VReg payload_val = lowerExpr(expr->payload);
        // Untagged: payload at offset 0; tagged: payload at offset 8
        uint32_t payload_offset = is_repr_c ? 0 : 8;

        VReg payload_ptr = freshVReg();
        LIRInstr fp2{};
        fp2.op = LIROp::FieldPtr;
        fp2.result = payload_ptr;
        fp2.type = ctx_.types.makePtr(expr->payload->type, false);
        fp2.field_ptr.base = base;
        fp2.field_ptr.offset = payload_offset;
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
            // For struct/array types, var_addrs_ holds a pointer to a pointer-slot
            // (the slot stores the struct's base address), so load to get actual addr
            TypeId operand_type = expr->operand->type;
            if (operand_type < ctx_.types.size()) {
                const auto& ti = ctx_.types.get(operand_type);
                if (ti.kind == TypeKind::Struct || ti.kind == TypeKind::Array) {
                    VReg base = freshVReg();
                    LIRInstr load{};
                    load.op = LIROp::Load;
                    load.result = base;
                    // Use Ptr type (the addr_of result type) so the backend
                    // treats this as a pointer load, not a struct copy.
                    load.type = expr->type;
                    load.load.ptr = var_it->second;
                    load.loc = expr->loc;
                    emit(load);
                    return base;
                }
            }
            // For primitive types, the var_addr IS the data address
            return var_it->second;
        }
    }

    // For &arr[i], compute the element pointer directly without loading the value
    if (expr->operand->kind == HIRExpr::Kind::IndexAccess) {
        auto* idx_expr = static_cast<const HIRIndexAccessExpr*>(expr->operand);
        return lowerIndexElementPtr(idx_expr);
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
            auto gv_it = global_label_map_.find(s->name);
            if (gv_it != global_label_map_.end()) {
                LIRInstr sg{};
                sg.op = LIROp::StoreGlobal;
                sg.result = INVALID_VREG;
                sg.type = s->value->type;
                sg.store_global.label = s->name;
                sg.store_global.value = val;
                sg.loc = s->loc;
                emit(sg);
            } else {
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

                auto* fi = lookupField(ctx_.types, fa->object->type, fa->field_name);
                if (fi && fi->bit_width > 0) {
                    // Bitfield write: read-modify-write
                    TypeId unit_type = fi->type;

                    VReg fp = freshVReg();
                    LIRInstr fp_instr{};
                    fp_instr.op = LIROp::FieldPtr;
                    fp_instr.result = fp;
                    fp_instr.type = ctx_.types.makePtr(unit_type, true);
                    fp_instr.field_ptr.base = obj;
                    fp_instr.field_ptr.offset = offset;
                    fp_instr.loc = s->loc;
                    emit(fp_instr);

                    // Load current storage unit
                    VReg old_val = freshVReg();
                    LIRInstr ld{};
                    ld.op = LIROp::Load;
                    ld.result = old_val;
                    ld.type = unit_type;
                    ld.load.ptr = fp;
                    emit(ld);

                    // Clear target bits: old & ~(mask << bit_offset)
                    uint64_t field_mask = (fi->bit_width >= 64) ? ~0ULL
                                          : ((1ULL << fi->bit_width) - 1);
                    uint64_t clear_mask = ~(field_mask << fi->bit_offset);
                    VReg cmask = freshVReg();
                    LIRInstr cm{};
                    cm.op = LIROp::ConstInt;
                    cm.result = cmask;
                    cm.type = unit_type;
                    cm.const_int.value = static_cast<int64_t>(clear_mask);
                    emit(cm);

                    VReg cleared = freshVReg();
                    LIRInstr ba{};
                    ba.op = LIROp::BAnd;
                    ba.result = cleared;
                    ba.type = unit_type;
                    ba.bin.lhs = old_val;
                    ba.bin.rhs = cmask;
                    emit(ba);

                    // Shift new value: val << bit_offset
                    VReg shifted_val = val;
                    if (fi->bit_offset > 0) {
                        VReg sh_amt = freshVReg();
                        LIRInstr sha{};
                        sha.op = LIROp::ConstInt;
                        sha.result = sh_amt;
                        sha.type = unit_type;
                        sha.const_int.value = fi->bit_offset;
                        emit(sha);

                        VReg sv = freshVReg();
                        LIRInstr shl{};
                        shl.op = LIROp::Shl;
                        shl.result = sv;
                        shl.type = unit_type;
                        shl.bin.lhs = val;
                        shl.bin.rhs = sh_amt;
                        emit(shl);
                        shifted_val = sv;
                    }

                    // Insert bits: cleared | shifted_val
                    VReg inserted = freshVReg();
                    LIRInstr bo{};
                    bo.op = LIROp::BOr;
                    bo.result = inserted;
                    bo.type = unit_type;
                    bo.bin.lhs = cleared;
                    bo.bin.rhs = shifted_val;
                    emit(bo);

                    // Store back
                    LIRInstr store{};
                    store.op = LIROp::Store;
                    store.result = INVALID_VREG;
                    store.type = TypeTable::Unit;
                    store.store.ptr = fp;
                    store.store.value = inserted;
                    store.loc = s->loc;
                    emit(store);
                } else {
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

VReg LIRBuilder::lowerIndexElementPtr(const HIRIndexAccessExpr* expr) {
    VReg base = lowerExpr(expr->array);
    VReg idx = lowerExpr(expr->index);

    // Get element type and size from array or pointer type
    TypeId arr_type = expr->array->type;
    uint32_t elem_size = 8; // default
    if (arr_type < ctx_.types.size()) {
        const auto& ti = ctx_.types.get(arr_type);
        if (ti.kind == TypeKind::Array) {
            elem_size = ctx_.types.sizeOf(ti.array.element);
        } else if (ti.kind == TypeKind::Ptr || ti.kind == TypeKind::PtrMut) {
            elem_size = ctx_.types.sizeOf(ti.ptr.pointee);
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

    return elem_ptr;
}

VReg LIRBuilder::lowerIndexAccess(const HIRIndexAccessExpr* expr) {
    VReg elem_ptr = lowerIndexElementPtr(expr);

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

    // Propagate constraint info
    instr.inline_asm.output_count = expr->output_count;
    instr.inline_asm.input_count = expr->input_count;
    instr.inline_asm.clobber_count = expr->clobber_count;
    // For inputs: load the variable's value and pass the loaded vreg
    // Check var_addrs_ first (mutable vars use pointer indirection),
    // then locals_ (val bindings hold SSA values directly).
    if (expr->input_count > 0) {
        instr.inline_asm.inputs = ctx_.arena.makeArray<LIRAsmOperand>(expr->input_count);
        for (uint32_t i = 0; i < expr->input_count; ++i) {
            instr.inline_asm.inputs[i].constraint = expr->inputs[i].constraint;
            auto va = var_addrs_.find(expr->inputs[i].var_name);
            if (va != var_addrs_.end()) {
                // Mutable var: load value through pointer
                VReg loaded = freshVReg();
                LIRInstr load{};
                load.op = LIROp::Load;
                load.result = loaded;
                load.type = TypeTable::I64;
                load.load.ptr = va->second;
                emit(load);
                instr.inline_asm.inputs[i].vreg = loaded;
            } else {
                auto it = locals_.find(expr->inputs[i].var_name);
                if (it != locals_.end()) {
                    // Val binding: use SSA value directly
                    instr.inline_asm.inputs[i].vreg = it->second;
                } else {
                    instr.inline_asm.inputs[i].vreg = INVALID_VREG;
                }
            }
        }
    } else { instr.inline_asm.inputs = nullptr; }

    // For outputs: create fresh vregs to receive the results
    // After the asm, we'll store these to the output variables.
    // Check var_addrs_ first (mutable vars), then locals_.
    struct OutputInfo { VReg out_vreg; VReg var_ptr; };
    auto* output_info = (expr->output_count > 0)
        ? ctx_.arena.makeArray<OutputInfo>(expr->output_count) : nullptr;
    if (expr->output_count > 0) {
        instr.inline_asm.outputs = ctx_.arena.makeArray<LIRAsmOperand>(expr->output_count);
        for (uint32_t i = 0; i < expr->output_count; ++i) {
            instr.inline_asm.outputs[i].constraint = expr->outputs[i].constraint;
            VReg out_vreg = freshVReg();
            instr.inline_asm.outputs[i].vreg = out_vreg;
            auto va = var_addrs_.find(expr->outputs[i].var_name);
            output_info[i].out_vreg = out_vreg;
            if (va != var_addrs_.end()) {
                output_info[i].var_ptr = va->second;
            } else {
                auto it = locals_.find(expr->outputs[i].var_name);
                output_info[i].var_ptr = (it != locals_.end()) ? it->second : INVALID_VREG;
            }
        }
    } else { instr.inline_asm.outputs = nullptr; }
    if (expr->clobber_count > 0) {
        instr.inline_asm.clobbers = ctx_.arena.makeArray<std::string_view>(expr->clobber_count);
        for (uint32_t i = 0; i < expr->clobber_count; ++i)
            instr.inline_asm.clobbers[i] = expr->clobbers[i];
    } else { instr.inline_asm.clobbers = nullptr; }

    instr.loc = expr->loc;
    emit(instr);

    // Store asm outputs back to their variables
    for (uint32_t i = 0; i < expr->output_count; ++i) {
        if (output_info[i].var_ptr != INVALID_VREG) {
            LIRInstr store{};
            store.op = LIROp::Store;
            store.result = INVALID_VREG;
            store.type = TypeTable::Unit;
            store.store.ptr = output_info[i].var_ptr;
            store.store.value = output_info[i].out_vreg;
            emit(store);
        }
    }

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
