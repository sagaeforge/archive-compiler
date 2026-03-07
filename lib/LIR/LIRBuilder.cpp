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
    return op == LIROp::Ret || op == LIROp::Branch || op == LIROp::CondBranch || op == LIROp::Trap;
}

static MemOrder extractMemOrder(const HIRExpr* expr) {
    if (expr->kind == HIRExpr::Kind::IntLit) {
        auto val = static_cast<const HIRIntLitExpr*>(expr)->value;
        if (val >= 0 && val <= 4) return static_cast<MemOrder>(val);
    }
    return MemOrder::SeqCst;
}

VReg LIRBuilder::freshVReg() {
    return next_vreg_++;
}

void LIRBuilder::emit(LIRInstr instr) {
    blocks_[current_block_].instrs.push_back(instr);
}

bool LIRBuilder::isAggregate(TypeId type) const {
    if (type >= ctx_.types.size()) return false;
    const auto& ti = ctx_.types.get(type);
    return ti.kind == TypeKind::Struct || ti.kind == TypeKind::Union;
}

void LIRBuilder::emitStructCopy(VReg dst_ptr, VReg src_ptr, uint32_t byte_size, SourceLocation loc) {
    // For large structs (>= 64 bytes), use rep movsq for efficiency
    static constexpr uint32_t REP_MOVSQ_THRESHOLD = 64;
    uint32_t aligned = (byte_size + 7u) & ~7u;

    if (aligned >= REP_MOVSQ_THRESHOLD) {
        // Emit inline asm: mov rdi, dst; mov rsi, src; mov rcx, count; rep movsq
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = INVALID_VREG;
        i.type = TypeTable::Unit;
        i.loc = loc;

        uint32_t qwords = aligned / 8;
        // Build asm lines
        auto* lines = ctx_.arena.makeArray<const char*>(4);
        auto* lens = ctx_.arena.makeArray<uint32_t>(4);

        // "mov rdi, $0" — dst pointer (output constraint)
        // "mov rsi, $1" — src pointer (input constraint)
        // "mov rcx, <count>"
        // "rep movsq"
        // We use inline asm with inputs for src/dst

        // Simpler: build a count string
        char count_buf[32];
        int count_len = snprintf(count_buf, sizeof(count_buf), "mov rcx, %u", qwords);
        char* count_str = ctx_.arena.makeArray<char>(static_cast<uint32_t>(count_len + 1));
        memcpy(count_str, count_buf, static_cast<size_t>(count_len + 1));

        lines[0] = "cld"; lens[0] = 3;
        lines[1] = count_str; lens[1] = static_cast<uint32_t>(count_len);
        lines[2] = "rep movsq"; lens[2] = 9;
        lines[3] = ""; lens[3] = 0; // sentinel (unused but safe)

        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 3;

        // Inputs: rdi = dst, rsi = src
        auto* inputs = ctx_.arena.makeArray<LIRAsmOperand>(2);
        inputs[0].vreg = dst_ptr;
        inputs[0].constraint = "D"; // rdi
        inputs[1].vreg = src_ptr;
        inputs[1].constraint = "S"; // rsi
        i.inline_asm.inputs = inputs;
        i.inline_asm.input_count = 2;

        i.inline_asm.outputs = nullptr;
        i.inline_asm.output_count = 0;

        // Clobbers: rcx, rdi, rsi (modified by rep movsq)
        auto* clobbers = ctx_.arena.makeArray<std::string_view>(3);
        clobbers[0] = "rcx";
        clobbers[1] = "rdi";
        clobbers[2] = "rsi";
        i.inline_asm.clobbers = clobbers;
        i.inline_asm.clobber_count = 3;

        emit(i);
        return;
    }

    for (uint32_t off = 0; off < aligned; off += 8) {
        // src_field = FieldPtr(src_ptr, off)
        VReg src_field = freshVReg();
        LIRInstr sp{};
        sp.op = LIROp::FieldPtr;
        sp.result = src_field;
        sp.type = ctx_.types.makePtr(TypeTable::I64, false);
        sp.field_ptr.base = src_ptr;
        sp.field_ptr.offset = off;
        sp.loc = loc;
        emit(sp);

        // tmp = Load(src_field)
        VReg tmp = freshVReg();
        LIRInstr ld{};
        ld.op = LIROp::Load;
        ld.result = tmp;
        ld.type = TypeTable::I64;
        ld.load.ptr = src_field;
        ld.loc = loc;
        emit(ld);

        // dst_field = FieldPtr(dst_ptr, off)
        VReg dst_field = freshVReg();
        LIRInstr dp{};
        dp.op = LIROp::FieldPtr;
        dp.result = dst_field;
        dp.type = ctx_.types.makePtr(TypeTable::I64, true);
        dp.field_ptr.base = dst_ptr;
        dp.field_ptr.offset = off;
        dp.loc = loc;
        emit(dp);

        // Store(dst_field, tmp)
        LIRInstr st{};
        st.op = LIROp::Store;
        st.result = INVALID_VREG;
        st.type = TypeTable::Unit;
        st.store.ptr = dst_field;
        st.store.value = tmp;
        st.loc = loc;
        emit(st);
    }
}

VReg LIRBuilder::lowerToAddress(const HIRExpr* expr) {
    if (expr->kind == HIRExpr::Kind::Ident) {
        auto* ident = static_cast<const HIRIdentExpr*>(expr);
        auto va = var_addrs_.find(ident->name);
        if (va != var_addrs_.end()) {
            // var binding: var slot stores a pointer to struct data
            if (isAggregate(expr->type)) {
                // Load the data pointer from the var slot
                VReg data_ptr = freshVReg();
                LIRInstr ld{};
                ld.op = LIROp::Load;
                ld.result = data_ptr;
                ld.type = ctx_.types.makePtr(expr->type, true);
                ld.load.ptr = va->second;
                ld.loc = expr->loc;
                emit(ld);
                return data_ptr;
            }
            // Primitive var: the var slot IS the address
            return va->second;
        }
        auto it = locals_.find(ident->name);
        if (it != locals_.end()) {
            return it->second; // val struct binding: vreg holds pointer to data
        }
    }
    if (expr->kind == HIRExpr::Kind::FieldAccess) {
        auto* fa = static_cast<const HIRFieldAccessExpr*>(expr);
        VReg base = lowerToAddress(fa->object);
        uint32_t offset = structFieldOffset(fa->object->type, fa->field_name);
        VReg fp = freshVReg();
        LIRInstr fp_instr{};
        fp_instr.op = LIROp::FieldPtr;
        fp_instr.result = fp;
        fp_instr.type = ctx_.types.makePtr(fa->type, true);
        fp_instr.field_ptr.base = base;
        fp_instr.field_ptr.offset = offset;
        fp_instr.loc = expr->loc;
        emit(fp_instr);
        return fp;
    }
    if (expr->kind == HIRExpr::Kind::Deref) {
        auto* deref = static_cast<const HIRDerefExpr*>(expr);
        return lowerExpr(deref->operand);
    }
    // Fallback: evaluate expression (creates a temporary)
    return lowerExpr(expr);
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
    // Propagate branch prediction hints from likely()/unlikely() builtins
    auto it = branch_hints_.find(cond);
    if (it != branch_hints_.end()) {
        i.cond_branch.branch_hint = it->second;
    }
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
        // Map source name → NASM label (link_name overrides for extern globals)
        auto* hgd = hir->globals[i];
        global_nasm_label_[hgd->name] = (!hgd->link_name.empty()) ? hgd->link_name : hgd->name;
    }

    // Pre-register vtable labels so lowerIdent can resolve them during fn building
    for (uint32_t i = 0; i < hir->vtable_count; ++i) {
        global_label_map_[hir->vtables[i].label] = hir->global_count + i;
        vtable_labels_.insert(hir->vtables[i].label);
    }

    // Pre-scan for variadic functions and panic handler
    for (uint32_t i = 0; i < hir->fn_count; ++i) {
        if (hir->functions[i]->is_variadic) {
            variadic_fns_.insert(hir->functions[i]->name);
        }
        if (hir->functions[i]->is_panic_handler) {
            panic_handler_name_ = hir->functions[i]->name;
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
        // Use link_name as NASM label for extern globals (matches extern declaration)
        gd.label = (!hgd->link_name.empty()) ? hgd->link_name : hgd->name;
        gd.variable.is_mutable = hgd->is_mutable;
        gd.variable.is_pub = hgd->is_pub;
        gd.variable.is_extern = hgd->is_extern;
        gd.variable.is_global_allocator = hgd->is_global_allocator;
        gd.variable.is_used = hgd->is_used;
        gd.variable.explicit_align = hgd->explicit_align;
        gd.variable.is_weak = hgd->is_weak;
        gd.variable.is_hidden = hgd->is_hidden;
        gd.variable.is_protected = hgd->is_protected;
        gd.variable.section_name = hgd->section_name;
        gd.variable.section_flags = hgd->section_flags;
        gd.variable.link_name = hgd->link_name;
        gd.variable.array_values = nullptr;
        gd.variable.array_labels = nullptr;
        gd.variable.array_count = 0;
        gd.variable.init_bytes = nullptr;
        gd.variable.init_byte_count = 0;
        gd.variable.relocs = nullptr;
        gd.variable.reloc_count = 0;
        // Evaluate constant initializer
        int64_t init_val = 0;
        if (hgd->init) {
            if (hgd->init->kind == HIRExpr::Kind::ArrayLit) {
                // Array literal: extract each element as a constant
                auto* arr = static_cast<const HIRArrayLitExpr*>(hgd->init);
                uint32_t n = arr->element_count;
                TypeId elem_type = TypeTable::I64;
                if (hgd->type_id < ctx_.types.size()) {
                    const auto& ti = ctx_.types.get(hgd->type_id);
                    if (ti.kind == TypeKind::Array) elem_type = ti.array.element;
                }
                uint32_t elem_sz = ctx_.types.sizeOf(elem_type);
                const auto& eti = ctx_.types.get(elem_type);
                bool elem_is_struct = (eti.kind == TypeKind::Struct);

                if (elem_is_struct) {
                    // Array of structs: serialize to raw bytes (init_bytes path)
                    uint32_t total_sz = elem_sz * n;
                    gd.variable.size = static_cast<uint8_t>(total_sz > 255 ? 255 : total_sz);
                    gd.variable.init_bytes = ctx_.arena.makeArray<uint8_t>(total_sz);
                    gd.variable.init_byte_count = total_sz;
                    std::memset(gd.variable.init_bytes, 0, total_sz);
                    for (uint32_t j = 0; j < n; ++j) {
                        uint32_t base_off = elem_sz * j;
                        if (arr->elements[j]->kind == HIRExpr::Kind::StructLit) {
                            auto* slit = static_cast<const HIRStructLitExpr*>(arr->elements[j]);
                            for (uint32_t fi = 0; fi < slit->field_count; ++fi) {
                                for (uint32_t si = 0; si < eti.struct_.field_count; ++si) {
                                    if (eti.struct_.fields[si].name == slit->fields[fi].name) {
                                        int32_t foff = eti.struct_.fields[si].offset;
                                        if (foff < 0) break;
                                        uint32_t fsz = ctx_.types.sizeOf(eti.struct_.fields[si].type);
                                        auto* fexpr = slit->fields[fi].value;
                                        if (fexpr->kind == HIRExpr::Kind::IntLit) {
                                            int64_t v = static_cast<const HIRIntLitExpr*>(fexpr)->value;
                                            std::memcpy(gd.variable.init_bytes + base_off + foff,
                                                       &v, std::min(fsz, 8u));
                                        } else if (fexpr->kind == HIRExpr::Kind::BoolLit) {
                                            uint8_t b = static_cast<const HIRBoolLitExpr*>(fexpr)->value ? 1 : 0;
                                            gd.variable.init_bytes[base_off + foff] = b;
                                        } else if (fexpr->kind == HIRExpr::Kind::FnRef) {
                                            auto* fr = static_cast<const HIRFnRefExpr*>(fexpr);
                                            uint32_t rc = gd.variable.reloc_count;
                                            auto* new_relocs = ctx_.arena.makeArray<GlobalReloc>(rc + 1);
                                            if (gd.variable.relocs && rc > 0) {
                                                std::memcpy(new_relocs, gd.variable.relocs,
                                                           rc * sizeof(GlobalReloc));
                                            }
                                            new_relocs[rc] = {base_off + static_cast<uint32_t>(foff),
                                                              fr->fn_name, 8, false};
                                            gd.variable.relocs = new_relocs;
                                            gd.variable.reloc_count = rc + 1;
                                        }
                                        break;
                                    }
                                }
                            }
                        }
                    }
                } else {
                    // Array of scalars (or fn pointers): use array_values path
                    gd.variable.array_count = n;
                    gd.variable.array_values = ctx_.arena.makeArray<int64_t>(n);
                    gd.variable.array_labels = nullptr;
                    gd.variable.size = static_cast<uint8_t>(elem_sz > 0 ? elem_sz : 8);
                    // Check if any element is a FnRef — if so, allocate labels array
                    bool has_fn_refs = false;
                    for (uint32_t j = 0; j < n; ++j) {
                        if (arr->elements[j]->kind == HIRExpr::Kind::FnRef) {
                            has_fn_refs = true;
                            break;
                        }
                    }
                    if (has_fn_refs) {
                        gd.variable.array_labels = ctx_.arena.makeArray<std::string_view>(n);
                        for (uint32_t j = 0; j < n; ++j) {
                            gd.variable.array_labels[j] = std::string_view{};
                        }
                    }
                    for (uint32_t j = 0; j < n; ++j) {
                        int64_t v = 0;
                        if (arr->elements[j]->kind == HIRExpr::Kind::IntLit) {
                            v = static_cast<const HIRIntLitExpr*>(arr->elements[j])->value;
                        } else if (arr->elements[j]->kind == HIRExpr::Kind::BoolLit) {
                            v = static_cast<const HIRBoolLitExpr*>(arr->elements[j])->value ? 1 : 0;
                        } else if (arr->elements[j]->kind == HIRExpr::Kind::FnRef) {
                            auto* fnref = static_cast<const HIRFnRefExpr*>(arr->elements[j]);
                            if (gd.variable.array_labels) {
                                gd.variable.array_labels[j] = fnref->fn_name;
                            }
                        }
                        gd.variable.array_values[j] = v;
                    }
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
                                    } else if (fexpr->kind == HIRExpr::Kind::FnRef) {
                                        // Function pointer field — add relocation
                                        auto* fr = static_cast<const HIRFnRefExpr*>(fexpr);
                                        // Grow relocation array
                                        uint32_t rc = gd.variable.reloc_count;
                                        auto* new_relocs = ctx_.arena.makeArray<GlobalReloc>(rc + 1);
                                        if (gd.variable.relocs && rc > 0) {
                                            std::memcpy(new_relocs, gd.variable.relocs,
                                                       rc * sizeof(GlobalReloc));
                                        }
                                        new_relocs[rc] = {static_cast<uint32_t>(off),
                                                          fr->fn_name, 8, false};
                                        gd.variable.relocs = new_relocs;
                                        gd.variable.reloc_count = rc + 1;
                                    }
                                    break;
                                }
                            }
                        }
                    }
                } else {
                    gd.variable.size = 8;
                }
            } else if (hgd->init->kind == HIRExpr::Kind::EnumAccess) {
                // Enum variant: serialize as the backing integer value
                auto* eacc = static_cast<const HIREnumAccessExpr*>(hgd->init);
                const auto& eti = ctx_.types.get(hgd->type_id);
                if (eti.kind == TypeKind::Enum) {
                    for (uint32_t vi = 0; vi < eti.enum_.variant_count; ++vi) {
                        if (eti.enum_.names[vi] == eacc->variant_name) {
                            init_val = eti.enum_.values[vi];
                            break;
                        }
                    }
                }
                uint32_t sz = ctx_.types.sizeOf(hgd->type_id);
                gd.variable.size = static_cast<uint8_t>(sz > 0 ? sz : 8);
            } else if (hgd->init->kind == HIRExpr::Kind::UnionVariant) {
                // Union variant literal: [tag: u64][payload...]
                auto* ulit = static_cast<const HIRUnionVariantExpr*>(hgd->init);
                uint32_t total_sz = ctx_.types.sizeOf(hgd->type_id);
                if (total_sz < 8) total_sz = 8; // at minimum, tag occupies 8 bytes
                gd.variable.size = static_cast<uint8_t>(total_sz > 255 ? 255 : total_sz);
                gd.variable.init_bytes = ctx_.arena.makeArray<uint8_t>(total_sz);
                gd.variable.init_byte_count = total_sz;
                std::memset(gd.variable.init_bytes, 0, total_sz);
                // Write discriminant tag
                const auto& uti = ctx_.types.get(hgd->type_id);
                uint64_t tag = 0;
                if (uti.kind == TypeKind::Union) {
                    for (uint32_t vi = 0; vi < uti.union_.variant_count; ++vi) {
                        if (uti.union_.variants[vi].name == ulit->variant_name) {
                            tag = vi;
                            break;
                        }
                    }
                }
                bool is_repr_c = (uti.kind == TypeKind::Union && uti.union_.is_repr_c);
                if (!is_repr_c) {
                    std::memcpy(gd.variable.init_bytes, &tag, 8);
                }
                // Write payload (if present)
                if (ulit->payload) {
                    uint32_t payload_offset = is_repr_c ? 0 : 8;
                    if (ulit->payload->kind == HIRExpr::Kind::IntLit) {
                        int64_t v = static_cast<const HIRIntLitExpr*>(ulit->payload)->value;
                        uint32_t pay_sz = total_sz - payload_offset;
                        std::memcpy(gd.variable.init_bytes + payload_offset, &v,
                                   std::min(pay_sz, 8u));
                    } else if (ulit->payload->kind == HIRExpr::Kind::BoolLit) {
                        uint8_t b = static_cast<const HIRBoolLitExpr*>(ulit->payload)->value ? 1 : 0;
                        gd.variable.init_bytes[payload_offset] = b;
                    } else if (ulit->payload->kind == HIRExpr::Kind::FloatLit) {
                        auto* fl = static_cast<const HIRFloatLitExpr*>(ulit->payload);
                        // Determine payload type from the variant
                        TypeId pay_type = TypeTable::F64;
                        if (uti.kind == TypeKind::Union && tag < uti.union_.variant_count) {
                            pay_type = uti.union_.variants[tag].payload_type;
                        }
                        if (pay_type == TypeTable::F32) {
                            float f = static_cast<float>(fl->value);
                            std::memcpy(gd.variable.init_bytes + payload_offset, &f, 4);
                        } else {
                            double d = fl->value;
                            std::memcpy(gd.variable.init_bytes + payload_offset, &d, 8);
                        }
                    }
                }
            } else if (hgd->init->kind == HIRExpr::Kind::FnRef) {
                // Top-level function pointer initializer: static val FP = some_fn
                auto* fr = static_cast<const HIRFnRefExpr*>(hgd->init);
                gd.variable.size = 8;
                gd.variable.init_bytes = ctx_.arena.makeArray<uint8_t>(8);
                gd.variable.init_byte_count = 8;
                std::memset(gd.variable.init_bytes, 0, 8);
                gd.variable.relocs = ctx_.arena.makeArray<GlobalReloc>(1);
                gd.variable.relocs[0] = {0, fr->fn_name, 8, false};
                gd.variable.reloc_count = 1;
            } else if (hgd->init->kind == HIRExpr::Kind::AddrOf) {
                // Address-of global: static val PTR = &OTHER_GLOBAL
                auto* ao = static_cast<const HIRAddrOfExpr*>(hgd->init);
                std::string_view target_name;
                if (ao->operand->kind == HIRExpr::Kind::Ident) {
                    target_name = static_cast<const HIRIdentExpr*>(ao->operand)->name;
                }
                gd.variable.size = 8;
                gd.variable.init_bytes = ctx_.arena.makeArray<uint8_t>(8);
                gd.variable.init_byte_count = 8;
                std::memset(gd.variable.init_bytes, 0, 8);
                if (!target_name.empty()) {
                    // Look up the NASM label for the target global
                    auto it = global_label_map_.find(target_name);
                    std::string_view label = target_name;
                    if (it != global_label_map_.end() && it->second < globals_.size()) {
                        label = globals_[it->second].label;
                    }
                    gd.variable.relocs = ctx_.arena.makeArray<GlobalReloc>(1);
                    gd.variable.relocs[0] = {0, label, 8, false};
                    gd.variable.reloc_count = 1;
                }
            } else {
                // Scalar integer/bool initializer
                if (hgd->init->kind == HIRExpr::Kind::IntLit) {
                    init_val = static_cast<const HIRIntLitExpr*>(hgd->init)->value;
                } else if (hgd->init->kind == HIRExpr::Kind::BoolLit) {
                    init_val = static_cast<const HIRBoolLitExpr*>(hgd->init)->value ? 1 : 0;
                } else if (hgd->init->kind == HIRExpr::Kind::UnaryOp) {
                    // Handle negation/bitnot of integer literal (e.g., -1)
                    auto* unary = static_cast<const HIRUnaryOpExpr*>(hgd->init);
                    if (unary->operand->kind == HIRExpr::Kind::IntLit) {
                        int64_t val = static_cast<const HIRIntLitExpr*>(unary->operand)->value;
                        if (unary->op == HIRUnaryOp::Neg) init_val = -val;
                        else if (unary->op == HIRUnaryOp::BitNot) init_val = ~val;
                    }
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
        gd.vtable.self_size = vt.self_size;
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
    current_param_count_ = fn->param_count;

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
    lir_fn.is_interrupt_error = fn->is_interrupt_error;
    lir_fn.is_interrupt_nofp = fn->is_interrupt_nofp;
    lir_fn.is_inline = fn->is_inline;
    lir_fn.is_noinline = fn->is_noinline;
    lir_fn.is_noreturn = fn->is_noreturn;
    lir_fn.is_pub = fn->is_pub;
    lir_fn.is_extern = fn->is_extern;
    lir_fn.is_variadic = fn->is_variadic;
    lir_fn.is_weak = fn->is_weak;
    lir_fn.is_cold = fn->is_cold;
    lir_fn.is_hot = fn->is_hot;
    lir_fn.is_hidden = fn->is_hidden;
    lir_fn.is_protected = fn->is_protected;
    lir_fn.is_constructor = fn->is_constructor;
    lir_fn.is_destructor = fn->is_destructor;
    lir_fn.constructor_priority = fn->constructor_priority;
    lir_fn.destructor_priority = fn->destructor_priority;
    lir_fn.is_used = fn->is_used;
    lir_fn.is_no_red_zone = fn->is_no_red_zone;
    lir_fn.fn_align = fn->fn_align;
    lir_fn.section_name = fn->section_name;
    lir_fn.section_flags = fn->section_flags;
    lir_fn.link_name = fn->link_name;
    lir_fn.loc = fn->loc;

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
        case HIRExpr::Kind::CStringLit:
            return lowerCStringLit(static_cast<const HIRCStringLitExpr*>(expr));
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

VReg LIRBuilder::lowerCStringLit(const HIRCStringLitExpr* expr) {
    // Add NUL-terminated string to globals (length + 1 bytes with \0)
    uint32_t gi = addStringGlobal(expr->data, expr->length + 1);

    VReg r = freshVReg();
    LIRInstr i{};
    i.op = LIROp::ConstCString;
    i.result = r;
    i.type = expr->type; // Ptr<u8>
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
        // Use NASM label (handles link_name for extern globals)
        auto nasm_it = global_nasm_label_.find(expr->name);
        i.load_global.label = (nasm_it != global_nasm_label_.end()) ? nasm_it->second : expr->name;
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

    // Branch prediction hints: likely(cond) → cond with +1 hint, unlikely(cond) → cond with -1 hint
    if ((callee == "likely" || callee == "unlikely") && expr->arg_count == 1) {
        VReg inner = lowerExpr(expr->args[0]);
        branch_hints_[inner] = (callee == "likely") ? int8_t(1) : int8_t(-1);
        return inner;
    }

    // va_start() -> *u8  (pointer to first variadic arg)
    if (callee == "va_start" && expr->arg_count == 0) {
        VReg r = freshVReg();
        LIRInstr i{};
        i.op = LIROp::VaStart;
        i.result = r;
        i.type = expr->type;
        i.va_start.fixed_param_count = current_param_count_;
        i.loc = expr->loc;
        emit(i);
        return r;
    }

    // va_arg(ap) -> T  (load next variadic arg from ap pointer)
    if (callee == "va_arg" && expr->arg_count == 1) {
        VReg ap = lowerExpr(expr->args[0]);
        VReg r = freshVReg();
        LIRInstr i{};
        i.op = LIROp::VaArg;
        i.result = r;
        i.type = expr->type;
        i.va_arg.ap = ap;
        i.loc = expr->loc;
        emit(i);
        return r;
    }

    // alloca(size) -> *u8  (dynamic stack allocation)
    if (callee == "alloca" && expr->arg_count == 1) {
        VReg sz = lowerExpr(expr->args[0]);
        VReg r = freshVReg();
        LIRInstr i{};
        i.op = LIROp::Alloca;
        i.result = r;
        i.type = expr->type;
        i.alloca_.size = sz;
        i.loc = expr->loc;
        emit(i);
        return r;
    }

    // atomic_load(ptr, order) -> value
    if (callee == "atomic_load" && expr->arg_count == 2) {
        VReg ptr = lowerExpr(expr->args[0]);
        MemOrder order = extractMemOrder(expr->args[1]);
        VReg r = freshVReg();
        LIRInstr i{};
        i.op = LIROp::AtomicLoad;
        i.result = r;
        i.type = expr->type;
        i.atomic_load.ptr = ptr;
        i.atomic_load.order = order;
        i.loc = expr->loc;
        emit(i);
        return r;
    }

    // atomic_store(ptr, value, order)
    if (callee == "atomic_store" && expr->arg_count >= 2) {
        VReg ptr = lowerExpr(expr->args[0]);
        VReg val = lowerExpr(expr->args[1]);
        MemOrder order = (expr->arg_count >= 3) ? extractMemOrder(expr->args[2]) : MemOrder::SeqCst;
        LIRInstr i{};
        i.op = LIROp::AtomicStore;
        i.result = INVALID_VREG;
        i.type = TypeTable::Unit;
        i.atomic_store.ptr = ptr;
        i.atomic_store.value = val;
        i.atomic_store.order = order;
        i.loc = expr->loc;
        emit(i);
        return INVALID_VREG;
    }

    // atomic_cas(ptr, expected, desired, order?) -> old value
    if (callee == "atomic_cas" && expr->arg_count >= 3) {
        VReg ptr = lowerExpr(expr->args[0]);
        VReg expected = lowerExpr(expr->args[1]);
        VReg desired = lowerExpr(expr->args[2]);
        MemOrder order = (expr->arg_count >= 4) ? extractMemOrder(expr->args[3]) : MemOrder::SeqCst;
        VReg r = freshVReg();
        LIRInstr i{};
        i.op = LIROp::AtomicCas;
        i.result = r;
        i.type = expr->type;
        i.atomic_cas.ptr = ptr;
        i.atomic_cas.expected = expected;
        i.atomic_cas.desired = desired;
        i.atomic_cas.order = order;
        i.loc = expr->loc;
        emit(i);
        return r;
    }

    // atomic_cas128(ptr, exp_lo, exp_hi, des_lo, des_hi, order?) -> bool
    if (callee == "atomic_cas128" && expr->arg_count >= 5) {
        VReg ptr = lowerExpr(expr->args[0]);
        VReg exp_lo = lowerExpr(expr->args[1]);
        VReg exp_hi = lowerExpr(expr->args[2]);
        VReg des_lo = lowerExpr(expr->args[3]);
        VReg des_hi = lowerExpr(expr->args[4]);
        MemOrder order = (expr->arg_count >= 6) ? extractMemOrder(expr->args[5]) : MemOrder::SeqCst;
        VReg r = freshVReg();
        LIRInstr i{};
        i.op = LIROp::AtomicCas128;
        i.result = r;
        i.type = TypeTable::Bool;
        i.atomic_cas128.ptr = ptr;
        i.atomic_cas128.exp_lo = exp_lo;
        i.atomic_cas128.exp_hi = exp_hi;
        i.atomic_cas128.des_lo = des_lo;
        i.atomic_cas128.des_hi = des_hi;
        i.atomic_cas128.order = order;
        i.loc = expr->loc;
        emit(i);
        return r;
    }

    // atomic_fetch_add(ptr, value, order?) -> old value
    if (callee == "atomic_fetch_add" && expr->arg_count >= 2) {
        VReg ptr = lowerExpr(expr->args[0]);
        VReg val = lowerExpr(expr->args[1]);
        MemOrder order = (expr->arg_count >= 3) ? extractMemOrder(expr->args[2]) : MemOrder::SeqCst;
        VReg r = freshVReg();
        LIRInstr i{};
        i.op = LIROp::AtomicFetchAdd;
        i.result = r;
        i.type = expr->type;
        i.atomic_fetch_add.ptr = ptr;
        i.atomic_fetch_add.value = val;
        i.atomic_fetch_add.order = order;
        i.loc = expr->loc;
        emit(i);
        return r;
    }

    // atomic_fetch_sub(ptr, value, order?) -> old value (lock xadd with negated value)
    if (callee == "atomic_fetch_sub" && expr->arg_count >= 2) {
        VReg ptr = lowerExpr(expr->args[0]);
        VReg val = lowerExpr(expr->args[1]);
        MemOrder order = (expr->arg_count >= 3) ? extractMemOrder(expr->args[2]) : MemOrder::SeqCst;
        VReg r = freshVReg();
        LIRInstr i{};
        i.op = LIROp::AtomicFetchSub;
        i.result = r;
        i.type = expr->type;
        i.atomic_fetch_sub.ptr = ptr;
        i.atomic_fetch_sub.value = val;
        i.atomic_fetch_sub.order = order;
        i.loc = expr->loc;
        emit(i);
        return r;
    }

    // atomic_fetch_and(ptr, value, order?) -> old value (CAS loop)
    if (callee == "atomic_fetch_and" && expr->arg_count >= 2) {
        VReg ptr = lowerExpr(expr->args[0]);
        VReg val = lowerExpr(expr->args[1]);
        MemOrder order = (expr->arg_count >= 3) ? extractMemOrder(expr->args[2]) : MemOrder::SeqCst;
        VReg r = freshVReg();
        LIRInstr i{};
        i.op = LIROp::AtomicFetchAnd;
        i.result = r;
        i.type = expr->type;
        i.atomic_rmw.ptr = ptr;
        i.atomic_rmw.value = val;
        i.atomic_rmw.order = order;
        i.loc = expr->loc;
        emit(i);
        return r;
    }

    // atomic_fetch_or(ptr, value, order?) -> old value (CAS loop)
    if (callee == "atomic_fetch_or" && expr->arg_count >= 2) {
        VReg ptr = lowerExpr(expr->args[0]);
        VReg val = lowerExpr(expr->args[1]);
        MemOrder order = (expr->arg_count >= 3) ? extractMemOrder(expr->args[2]) : MemOrder::SeqCst;
        VReg r = freshVReg();
        LIRInstr i{};
        i.op = LIROp::AtomicFetchOr;
        i.result = r;
        i.type = expr->type;
        i.atomic_rmw.ptr = ptr;
        i.atomic_rmw.value = val;
        i.atomic_rmw.order = order;
        i.loc = expr->loc;
        emit(i);
        return r;
    }

    // atomic_fetch_xor(ptr, value, order?) -> old value (CAS loop)
    if (callee == "atomic_fetch_xor" && expr->arg_count >= 2) {
        VReg ptr = lowerExpr(expr->args[0]);
        VReg val = lowerExpr(expr->args[1]);
        MemOrder order = (expr->arg_count >= 3) ? extractMemOrder(expr->args[2]) : MemOrder::SeqCst;
        VReg r = freshVReg();
        LIRInstr i{};
        i.op = LIROp::AtomicFetchXor;
        i.result = r;
        i.type = expr->type;
        i.atomic_rmw.ptr = ptr;
        i.atomic_rmw.value = val;
        i.atomic_rmw.order = order;
        i.loc = expr->loc;
        emit(i);
        return r;
    }

    // atomic_xchg(ptr, val) -> old value — uses xchg (implicitly locked)
    if (callee == "atomic_xchg" && expr->arg_count == 2) {
        VReg ptr = lowerExpr(expr->args[0]);
        VReg val = lowerExpr(expr->args[1]);
        VReg r = freshVReg();
        // xchg [$0], rax → old value in rax, uses generic "r" for ptr
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = r;
        i.type = expr->type;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(1);
        auto* lens = ctx_.arena.makeArray<uint32_t>(1);
        lines[0] = "xchg [$1], rax"; lens[0] = 14;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 1;
        auto* outs = ctx_.arena.makeArray<LIRAsmOperand>(1);
        outs[0].constraint = "=a"; outs[0].vreg = r;
        i.inline_asm.outputs = outs;
        i.inline_asm.output_count = 1;
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(2);
        ins[0].constraint = "r"; ins[0].vreg = ptr;
        ins[1].constraint = "a"; ins[1].vreg = val;
        i.inline_asm.inputs = ins;
        i.inline_asm.input_count = 2;
        auto* clobs = ctx_.arena.makeArray<std::string_view>(1);
        clobs[0] = "memory";
        i.inline_asm.clobbers = clobs;
        i.inline_asm.clobber_count = 1;
        emit(i);
        return r;
    }

    // Bit test intrinsics: bt/bts/btr/btc(ptr, bit_index) -> bool (previous bit value)
    if ((callee == "bt" || callee == "bts" || callee == "btr" || callee == "btc") &&
        expr->arg_count == 2) {
        VReg base = lowerExpr(expr->args[0]);
        VReg bit_idx = lowerExpr(expr->args[1]);
        VReg r = freshVReg();
        const char* asm_str = nullptr;
        uint32_t asm_len = 0;
        if (callee == "bt")  { asm_str = "bt [rcx], rdx";  asm_len = 13; }
        if (callee == "bts") { asm_str = "bts [rcx], rdx"; asm_len = 14; }
        if (callee == "btr") { asm_str = "btr [rcx], rdx"; asm_len = 14; }
        if (callee == "btc") { asm_str = "btc [rcx], rdx"; asm_len = 14; }
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = r;
        i.type = TypeTable::Bool;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(3);
        auto* lens = ctx_.arena.makeArray<uint32_t>(3);
        lines[0] = asm_str; lens[0] = asm_len;
        lines[1] = "setc al"; lens[1] = 7;
        lines[2] = "movzx rax, al"; lens[2] = 13;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 3;
        auto* outs = ctx_.arena.makeArray<LIRAsmOperand>(1);
        outs[0].constraint = "=a"; outs[0].vreg = r;
        i.inline_asm.outputs = outs;
        i.inline_asm.output_count = 1;
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(2);
        ins[0].constraint = "c"; ins[0].vreg = base;     // rcx = ptr
        ins[1].constraint = "d"; ins[1].vreg = bit_idx;  // rdx = bit index
        i.inline_asm.inputs = ins;
        i.inline_asm.input_count = 2;
        auto* clobs = ctx_.arena.makeArray<std::string_view>(2);
        clobs[0] = "rcx"; clobs[1] = "rdx";
        i.inline_asm.clobbers = clobs;
        i.inline_asm.clobber_count = 2;
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

    // tls_load(offset) -> value (FS segment, thread-local storage)
    if (callee == "tls_load" && expr->arg_count == 1) {
        VReg offset = lowerExpr(expr->args[0]);
        VReg r = freshVReg();
        LIRInstr i{};
        i.op = LIROp::TlsLoad;
        i.result = r;
        i.type = expr->type;
        i.tls_load.offset = offset;
        i.loc = expr->loc;
        emit(i);
        return r;
    }

    // tls_store(offset, value) (FS segment, thread-local storage)
    if (callee == "tls_store" && expr->arg_count == 2) {
        VReg offset = lowerExpr(expr->args[0]);
        VReg val = lowerExpr(expr->args[1]);
        LIRInstr i{};
        i.op = LIROp::TlsStore;
        i.result = INVALID_VREG;
        i.type = TypeTable::Unit;
        i.tls_store.offset = offset;
        i.tls_store.value = val;
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

    // debug_break() → int3 (software breakpoint)
    if (callee == "debug_break" && expr->arg_count == 0) {
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = INVALID_VREG;
        i.type = TypeTable::Unit;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(1);
        auto* lens = ctx_.arena.makeArray<uint32_t>(1);
        lines[0] = "int3"; lens[0] = 4;
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

    // panic() → call @panic_handler if registered, otherwise ud2
    if (callee == "panic" && expr->arg_count <= 1) {
        if (!panic_handler_name_.empty()) {
            // Call the registered @panic_handler function
            VReg msg = INVALID_VREG;
            uint32_t arg_count = 0;
            VReg* args = nullptr;
            if (expr->arg_count == 1) {
                msg = lowerExpr(expr->args[0]);
                args = ctx_.arena.makeArray<VReg>(1);
                args[0] = msg;
                arg_count = 1;
            }
            LIRInstr ci{};
            ci.op = LIROp::Call;
            ci.result = INVALID_VREG;
            ci.type = TypeTable::Never;
            ci.loc = expr->loc;
            ci.call.callee = panic_handler_name_;
            ci.call.args = args;
            ci.call.arg_count = arg_count;
            ci.call.is_tail = false;
            emit(ci);
            // Emit ud2 after handler call as safety net (handler should not return)
            LIRInstr trap{};
            trap.op = LIROp::Trap;
            trap.result = INVALID_VREG;
            trap.type = TypeTable::Never;
            trap.loc = expr->loc;
            emit(trap);
        } else {
            // No panic handler registered — emit ud2 directly
            if (expr->arg_count == 1) {
                lowerExpr(expr->args[0]); // lower for side effects
            }
            LIRInstr trap{};
            trap.op = LIROp::Trap;
            trap.result = INVALID_VREG;
            trap.type = TypeTable::Never;
            trap.loc = expr->loc;
            emit(trap);
        }
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
        if (callee == "io_wait") {
            // I/O wait: out 0x80, al — writes to port 0x80 for ~1μs delay
            LIRInstr i{};
            i.op = LIROp::InlineAsm;
            i.result = INVALID_VREG;
            i.type = TypeTable::Unit;
            i.loc = expr->loc;
            auto* lines2 = ctx_.arena.makeArray<const char*>(1);
            auto* lens2 = ctx_.arena.makeArray<uint32_t>(1);
            lines2[0] = "out 0x80, al"; lens2[0] = 12;
            i.inline_asm.lines = lines2;
            i.inline_asm.line_lengths = lens2;
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

    // ldmxcsr(val: u32) -> Unit — load SSE control register
    if (callee == "ldmxcsr" && expr->arg_count == 1) {
        VReg val = lowerExpr(expr->args[0]);
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = INVALID_VREG;
        i.type = TypeTable::Unit;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(3);
        auto* lens = ctx_.arena.makeArray<uint32_t>(3);
        lines[0] = "sub rsp, 4";     lens[0] = 10;
        lines[1] = "mov [rsp], $0";  lens[1] = 12;
        lines[2] = "ldmxcsr [rsp]";  lens[2] = 13;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 3;
        i.inline_asm.outputs = nullptr;
        i.inline_asm.output_count = 0;
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(1);
        ins[0].constraint = "r"; ins[0].vreg = val;
        i.inline_asm.inputs = ins;
        i.inline_asm.input_count = 1;
        i.inline_asm.clobbers = nullptr;
        i.inline_asm.clobber_count = 0;
        emit(i);
        // Restore rsp after ldmxcsr
        LIRInstr i2{};
        i2.op = LIROp::InlineAsm;
        i2.result = INVALID_VREG;
        i2.type = TypeTable::Unit;
        i2.loc = expr->loc;
        auto* lines2 = ctx_.arena.makeArray<const char*>(1);
        auto* lens2 = ctx_.arena.makeArray<uint32_t>(1);
        lines2[0] = "add rsp, 4"; lens2[0] = 10;
        i2.inline_asm.lines = lines2;
        i2.inline_asm.line_lengths = lens2;
        i2.inline_asm.line_count = 1;
        i2.inline_asm.outputs = nullptr;
        i2.inline_asm.output_count = 0;
        i2.inline_asm.inputs = nullptr;
        i2.inline_asm.input_count = 0;
        i2.inline_asm.clobbers = nullptr;
        i2.inline_asm.clobber_count = 0;
        emit(i2);
        return INVALID_VREG;
    }

    // stmxcsr(buf: Ptr<var u32>) -> Unit — store SSE control register
    if (callee == "stmxcsr" && expr->arg_count == 1) {
        VReg ptr = lowerExpr(expr->args[0]);
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = INVALID_VREG;
        i.type = TypeTable::Unit;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(1);
        auto* lens = ctx_.arena.makeArray<uint32_t>(1);
        lines[0] = "stmxcsr [$0]"; lens[0] = 12;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 1;
        i.inline_asm.outputs = nullptr;
        i.inline_asm.output_count = 0;
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(1);
        ins[0].constraint = "D"; ins[0].vreg = ptr;
        i.inline_asm.inputs = ins;
        i.inline_asm.input_count = 1;
        i.inline_asm.clobbers = nullptr;
        i.inline_asm.clobber_count = 0;
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

    // SSE intrinsics: sse_storeu_128(ptr, lo, hi) — store 128 bits unaligned
    if (callee == "sse_storeu_128" && expr->arg_count == 3) {
        VReg ptr = lowerExpr(expr->args[0]);
        VReg lo = lowerExpr(expr->args[1]);
        VReg hi = lowerExpr(expr->args[2]);
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = INVALID_VREG;
        i.type = TypeTable::Unit;
        i.loc = expr->loc;
        // movq xmm0, $1; movq xmm1, $2; punpcklqdq xmm0, xmm1; movdqu [$0], xmm0
        auto* lines = ctx_.arena.makeArray<const char*>(4);
        auto* lens = ctx_.arena.makeArray<uint32_t>(4);
        lines[0] = "movq xmm0, $1";          lens[0] = 14;
        lines[1] = "movq xmm1, $2";          lens[1] = 14;
        lines[2] = "punpcklqdq xmm0, xmm1";  lens[2] = 23;
        lines[3] = "movdqu [$0], xmm0";       lens[3] = 18;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 4;
        i.inline_asm.outputs = nullptr;
        i.inline_asm.output_count = 0;
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(3);
        ins[0].constraint = "r"; ins[0].vreg = ptr;
        ins[1].constraint = "r"; ins[1].vreg = lo;
        ins[2].constraint = "r"; ins[2].vreg = hi;
        i.inline_asm.inputs = ins;
        i.inline_asm.input_count = 3;
        i.inline_asm.clobbers = nullptr;
        i.inline_asm.clobber_count = 0;
        emit(i);
        return INVALID_VREG;
    }

    // sse_loadu_lo64(ptr) -> u64 — load low 64 bits from 128-bit address
    if (callee == "sse_loadu_lo64" && expr->arg_count == 1) {
        VReg ptr = lowerExpr(expr->args[0]);
        VReg result = freshVReg();
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = result;
        i.type = TypeTable::U64;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(2);
        auto* lens = ctx_.arena.makeArray<uint32_t>(2);
        lines[0] = "movdqu xmm0, [$0]";  lens[0] = 18;
        lines[1] = "movq $r, xmm0";      lens[1] = 13;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 2;
        auto* outs = ctx_.arena.makeArray<LIRAsmOperand>(1);
        outs[0].constraint = "=r"; outs[0].vreg = result;
        i.inline_asm.outputs = outs;
        i.inline_asm.output_count = 1;
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(1);
        ins[0].constraint = "r"; ins[0].vreg = ptr;
        i.inline_asm.inputs = ins;
        i.inline_asm.input_count = 1;
        i.inline_asm.clobbers = nullptr;
        i.inline_asm.clobber_count = 0;
        emit(i);
        return result;
    }

    // sse_loadu_hi64(ptr) -> u64 — load high 64 bits from 128-bit address
    if (callee == "sse_loadu_hi64" && expr->arg_count == 1) {
        VReg ptr = lowerExpr(expr->args[0]);
        VReg result = freshVReg();
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = result;
        i.type = TypeTable::U64;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(3);
        auto* lens = ctx_.arena.makeArray<uint32_t>(3);
        lines[0] = "movdqu xmm0, [$0]";      lens[0] = 18;
        lines[1] = "psrldq xmm0, 8";         lens[1] = 14;
        lines[2] = "movq $r, xmm0";          lens[2] = 13;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 3;
        auto* outs = ctx_.arena.makeArray<LIRAsmOperand>(1);
        outs[0].constraint = "=r"; outs[0].vreg = result;
        i.inline_asm.outputs = outs;
        i.inline_asm.output_count = 1;
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(1);
        ins[0].constraint = "r"; ins[0].vreg = ptr;
        i.inline_asm.inputs = ins;
        i.inline_asm.input_count = 1;
        i.inline_asm.clobbers = nullptr;
        i.inline_asm.clobber_count = 0;
        emit(i);
        return result;
    }

    // sse_zero_128(ptr) — write 128 zero bits using pxor + movdqu
    if (callee == "sse_zero_128" && expr->arg_count == 1) {
        VReg ptr = lowerExpr(expr->args[0]);
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = INVALID_VREG;
        i.type = TypeTable::Unit;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(2);
        auto* lens = ctx_.arena.makeArray<uint32_t>(2);
        lines[0] = "pxor xmm0, xmm0";    lens[0] = 16;
        lines[1] = "movdqu [$0], xmm0";   lens[1] = 18;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 2;
        i.inline_asm.outputs = nullptr;
        i.inline_asm.output_count = 0;
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(1);
        ins[0].constraint = "r"; ins[0].vreg = ptr;
        i.inline_asm.inputs = ins;
        i.inline_asm.input_count = 1;
        i.inline_asm.clobbers = nullptr;
        i.inline_asm.clobber_count = 0;
        emit(i);
        return INVALID_VREG;
    }

    // sse_copy_128(dst, src) — copy 128 bits using movdqu
    if (callee == "sse_copy_128" && expr->arg_count == 2) {
        VReg dst = lowerExpr(expr->args[0]);
        VReg src = lowerExpr(expr->args[1]);
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = INVALID_VREG;
        i.type = TypeTable::Unit;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(2);
        auto* lens = ctx_.arena.makeArray<uint32_t>(2);
        lines[0] = "movdqu xmm0, [$1]";   lens[0] = 18;
        lines[1] = "movdqu [$0], xmm0";    lens[1] = 18;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 2;
        i.inline_asm.outputs = nullptr;
        i.inline_asm.output_count = 0;
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(2);
        ins[0].constraint = "r"; ins[0].vreg = dst;
        ins[1].constraint = "r"; ins[1].vreg = src;
        i.inline_asm.inputs = ins;
        i.inline_asm.input_count = 2;
        i.inline_asm.clobbers = nullptr;
        i.inline_asm.clobber_count = 0;
        emit(i);
        return INVALID_VREG;
    }

    // AES-NI intrinsics: aesenc, aesenclast, aesdec, aesdeclast, aesimc
    // All: (state_ptr, key_ptr) — load xmm0/xmm1 from ptrs, perform op, store back
    if ((callee == "aesenc" || callee == "aesenclast" ||
         callee == "aesdec" || callee == "aesdeclast" ||
         callee == "aesimc") && expr->arg_count == 2) {
        VReg state_ptr = lowerExpr(expr->args[0]);
        VReg key_ptr = lowerExpr(expr->args[1]);
        const char* op_asm = nullptr;
        uint32_t op_len = 0;
        if (callee == "aesenc")      { op_asm = "aesenc xmm0, xmm1";      op_len = 17; }
        if (callee == "aesenclast")  { op_asm = "aesenclast xmm0, xmm1";  op_len = 21; }
        if (callee == "aesdec")      { op_asm = "aesdec xmm0, xmm1";      op_len = 17; }
        if (callee == "aesdeclast")  { op_asm = "aesdeclast xmm0, xmm1";  op_len = 21; }
        if (callee == "aesimc")      { op_asm = "aesimc xmm0, xmm1";      op_len = 17; }
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = INVALID_VREG;
        i.type = TypeTable::Unit;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(4);
        auto* lens = ctx_.arena.makeArray<uint32_t>(4);
        lines[0] = "movdqu xmm0, [$0]";  lens[0] = 18;
        lines[1] = "movdqu xmm1, [$1]";  lens[1] = 18;
        lines[2] = op_asm;                lens[2] = op_len;
        lines[3] = "movdqu [$0], xmm0";   lens[3] = 18;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 4;
        i.inline_asm.outputs = nullptr;
        i.inline_asm.output_count = 0;
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(2);
        ins[0].constraint = "r"; ins[0].vreg = state_ptr;
        ins[1].constraint = "r"; ins[1].vreg = key_ptr;
        i.inline_asm.inputs = ins;
        i.inline_asm.input_count = 2;
        i.inline_asm.clobbers = nullptr;
        i.inline_asm.clobber_count = 0;
        emit(i);
        return INVALID_VREG;
    }

    // aeskeygenassist(dst, src, rcon)
    if (callee == "aeskeygenassist" && expr->arg_count == 3) {
        VReg dst = lowerExpr(expr->args[0]);
        VReg src = lowerExpr(expr->args[1]);
        VReg rcon = lowerExpr(expr->args[2]);
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = INVALID_VREG;
        i.type = TypeTable::Unit;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(3);
        auto* lens = ctx_.arena.makeArray<uint32_t>(3);
        lines[0] = "movdqu xmm1, [$1]";                 lens[0] = 18;
        lines[1] = "aeskeygenassist xmm0, xmm1, $2";    lens[1] = 30;
        lines[2] = "movdqu [$0], xmm0";                  lens[2] = 18;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 3;
        i.inline_asm.outputs = nullptr;
        i.inline_asm.output_count = 0;
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(3);
        ins[0].constraint = "r"; ins[0].vreg = dst;
        ins[1].constraint = "r"; ins[1].vreg = src;
        ins[2].constraint = "r"; ins[2].vreg = rcon;
        i.inline_asm.inputs = ins;
        i.inline_asm.input_count = 3;
        i.inline_asm.clobbers = nullptr;
        i.inline_asm.clobber_count = 0;
        emit(i);
        return INVALID_VREG;
    }

    // pclmulqdq(dst, a, b, imm) — carry-less multiplication
    if (callee == "pclmulqdq" && expr->arg_count == 4) {
        VReg dst_ptr = lowerExpr(expr->args[0]);
        VReg a_ptr = lowerExpr(expr->args[1]);
        VReg b_ptr = lowerExpr(expr->args[2]);
        VReg imm = lowerExpr(expr->args[3]);
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = INVALID_VREG;
        i.type = TypeTable::Unit;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(4);
        auto* lens = ctx_.arena.makeArray<uint32_t>(4);
        lines[0] = "movdqu xmm0, [$1]";          lens[0] = 18;
        lines[1] = "movdqu xmm1, [$2]";          lens[1] = 18;
        lines[2] = "pclmulqdq xmm0, xmm1, $3";   lens[2] = 26;
        lines[3] = "movdqu [$0], xmm0";           lens[3] = 18;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 4;
        i.inline_asm.outputs = nullptr;
        i.inline_asm.output_count = 0;
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(4);
        ins[0].constraint = "r"; ins[0].vreg = dst_ptr;
        ins[1].constraint = "r"; ins[1].vreg = a_ptr;
        ins[2].constraint = "r"; ins[2].vreg = b_ptr;
        ins[3].constraint = "r"; ins[3].vreg = imm;
        i.inline_asm.inputs = ins;
        i.inline_asm.input_count = 4;
        i.inline_asm.clobbers = nullptr;
        i.inline_asm.clobber_count = 0;
        emit(i);
        return INVALID_VREG;
    }

    // SHA-NI: sha256rnds2(state, msg, extra), sha1rnds4(state, msg, fn_idx)
    if ((callee == "sha256rnds2" || callee == "sha1rnds4") && expr->arg_count == 3) {
        VReg state_ptr = lowerExpr(expr->args[0]);
        VReg msg_ptr = lowerExpr(expr->args[1]);
        VReg extra = lowerExpr(expr->args[2]);
        const char* op_asm = nullptr;
        uint32_t op_len = 0;
        if (callee == "sha256rnds2") {
            op_asm = "sha256rnds2 xmm0, xmm1";
            op_len = 23;
        } else {
            op_asm = "sha1rnds4 xmm0, xmm1, $2";
            op_len = 26;
        }
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = INVALID_VREG;
        i.type = TypeTable::Unit;
        i.loc = expr->loc;
        // For sha256rnds2: xmm0=state, xmm1=msg, xmm0 implicit in instruction
        // Load xmm0 from state, xmm1 from msg, extra goes to xmm2 (for sha256rnds2, the implicit xmm0)
        auto* lines = ctx_.arena.makeArray<const char*>(4);
        auto* lens = ctx_.arena.makeArray<uint32_t>(4);
        lines[0] = "movdqu xmm0, [$0]";   lens[0] = 18;
        lines[1] = "movdqu xmm1, [$1]";   lens[1] = 18;
        lines[2] = op_asm;                 lens[2] = op_len;
        lines[3] = "movdqu [$0], xmm0";    lens[3] = 18;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 4;
        i.inline_asm.outputs = nullptr;
        i.inline_asm.output_count = 0;
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(3);
        ins[0].constraint = "r"; ins[0].vreg = state_ptr;
        ins[1].constraint = "r"; ins[1].vreg = msg_ptr;
        ins[2].constraint = "r"; ins[2].vreg = extra;
        i.inline_asm.inputs = ins;
        i.inline_asm.input_count = 3;
        i.inline_asm.clobbers = nullptr;
        i.inline_asm.clobber_count = 0;
        emit(i);
        return INVALID_VREG;
    }

    // sha256msg1/sha256msg2(state, msg)
    if ((callee == "sha256msg1" || callee == "sha256msg2") && expr->arg_count == 2) {
        VReg state_ptr = lowerExpr(expr->args[0]);
        VReg msg_ptr = lowerExpr(expr->args[1]);
        const char* op_asm = nullptr;
        uint32_t op_len = 0;
        if (callee == "sha256msg1") { op_asm = "sha256msg1 xmm0, xmm1"; op_len = 22; }
        else                        { op_asm = "sha256msg2 xmm0, xmm1"; op_len = 22; }
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = INVALID_VREG;
        i.type = TypeTable::Unit;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(4);
        auto* lens = ctx_.arena.makeArray<uint32_t>(4);
        lines[0] = "movdqu xmm0, [$0]";   lens[0] = 18;
        lines[1] = "movdqu xmm1, [$1]";   lens[1] = 18;
        lines[2] = op_asm;                 lens[2] = op_len;
        lines[3] = "movdqu [$0], xmm0";    lens[3] = 18;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 4;
        i.inline_asm.outputs = nullptr;
        i.inline_asm.output_count = 0;
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(2);
        ins[0].constraint = "r"; ins[0].vreg = state_ptr;
        ins[1].constraint = "r"; ins[1].vreg = msg_ptr;
        i.inline_asm.inputs = ins;
        i.inline_asm.input_count = 2;
        i.inline_asm.clobbers = nullptr;
        i.inline_asm.clobber_count = 0;
        emit(i);
        return INVALID_VREG;
    }

    // crc32c(acc: u32, data: u32) -> u32
    if (callee == "crc32c" && expr->arg_count == 2) {
        VReg acc = lowerExpr(expr->args[0]);
        VReg data = lowerExpr(expr->args[1]);
        VReg result = freshVReg();
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = result;
        i.type = TypeTable::U32;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(2);
        auto* lens = ctx_.arena.makeArray<uint32_t>(2);
        lines[0] = "mov $r, $0";       lens[0] = 9;
        lines[1] = "crc32 $r, $1";     lens[1] = 11;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 2;
        auto* outs = ctx_.arena.makeArray<LIRAsmOperand>(1);
        outs[0].constraint = "=r"; outs[0].vreg = result;
        i.inline_asm.outputs = outs;
        i.inline_asm.output_count = 1;
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(2);
        ins[0].constraint = "r"; ins[0].vreg = acc;
        ins[1].constraint = "r"; ins[1].vreg = data;
        i.inline_asm.inputs = ins;
        i.inline_asm.input_count = 2;
        i.inline_asm.clobbers = nullptr;
        i.inline_asm.clobber_count = 0;
        emit(i);
        return result;
    }

    // Cache management + prefetch intrinsics (ptr → Unit)
    if (expr->arg_count == 1 && expr->type == TypeTable::Unit) {
        const char* cache_asm = nullptr;
        uint32_t cache_len = 0;
        if (callee == "clflush")      { cache_asm = "clflush [$0]";      cache_len = 12; }
        if (callee == "clflushopt")   { cache_asm = "clflushopt [$0]";   cache_len = 15; }
        if (callee == "clwb")         { cache_asm = "clwb [$0]";         cache_len = 9; }
        if (callee == "prefetcht0")   { cache_asm = "prefetcht0 [$0]";   cache_len = 15; }
        if (callee == "prefetcht1")   { cache_asm = "prefetcht1 [$0]";   cache_len = 15; }
        if (callee == "prefetcht2")   { cache_asm = "prefetcht2 [$0]";   cache_len = 15; }
        if (callee == "prefetchnta")  { cache_asm = "prefetchnta [$0]";  cache_len = 16; }
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

    // memmove(dst, src, count) — handles overlapping regions
    if (callee == "memmove" && expr->arg_count == 3) {
        VReg dst = lowerExpr(expr->args[0]);
        VReg src = lowerExpr(expr->args[1]);
        VReg cnt = lowerExpr(expr->args[2]);

        // Compare dst and src to determine copy direction
        VReg cmp_val = freshVReg();
        LIRInstr cmp_instr{};
        cmp_instr.op = LIROp::ICmpLt;  // dst < src?
        cmp_instr.result = cmp_val;
        cmp_instr.type = TypeTable::Bool;
        cmp_instr.bin.lhs = dst;
        cmp_instr.bin.rhs = src;
        cmp_instr.loc = expr->loc;
        emit(cmp_instr);

        uint32_t fwd_bb = newBlock("memmove_fwd");
        uint32_t bwd_bb = newBlock("memmove_bwd");
        uint32_t done_bb = newBlock("memmove_done");

        emitCondBranch(cmp_val, fwd_bb, bwd_bb);

        // Forward copy: cld; rep movsb (dst < src, no overlap concern)
        switchToBlock(fwd_bb);
        {
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
            ins[0].constraint = "D"; ins[0].vreg = dst;
            ins[1].constraint = "S"; ins[1].vreg = src;
            ins[2].constraint = "c"; ins[2].vreg = cnt;
            i.inline_asm.inputs = ins;
            i.inline_asm.input_count = 3;
            auto* clobs = ctx_.arena.makeArray<std::string_view>(3);
            clobs[0] = "rdi"; clobs[1] = "rsi"; clobs[2] = "rcx";
            i.inline_asm.clobbers = clobs;
            i.inline_asm.clobber_count = 3;
            emit(i);
        }
        emitBranch(done_bb);

        // Backward copy: adjust pointers to end, std; rep movsb; cld
        switchToBlock(bwd_bb);
        {
            // dst_end = dst + count - 1
            VReg one = freshVReg();
            LIRInstr one_i{};
            one_i.op = LIROp::ConstInt; one_i.result = one;
            one_i.type = TypeTable::I64; one_i.const_int.value = 1;
            emit(one_i);

            VReg cnt_m1 = freshVReg();
            LIRInstr sub1{};
            sub1.op = LIROp::Sub; sub1.result = cnt_m1;
            sub1.type = TypeTable::I64; sub1.bin.lhs = cnt; sub1.bin.rhs = one;
            emit(sub1);

            VReg dst_end = freshVReg();
            LIRInstr add_d{};
            add_d.op = LIROp::Add; add_d.result = dst_end;
            add_d.type = TypeTable::I64; add_d.bin.lhs = dst; add_d.bin.rhs = cnt_m1;
            emit(add_d);

            VReg src_end = freshVReg();
            LIRInstr add_s{};
            add_s.op = LIROp::Add; add_s.result = src_end;
            add_s.type = TypeTable::I64; add_s.bin.lhs = src; add_s.bin.rhs = cnt_m1;
            emit(add_s);

            LIRInstr i{};
            i.op = LIROp::InlineAsm;
            i.result = INVALID_VREG;
            i.type = TypeTable::Unit;
            i.loc = expr->loc;
            auto* lines = ctx_.arena.makeArray<const char*>(3);
            auto* lens = ctx_.arena.makeArray<uint32_t>(3);
            lines[0] = "std"; lens[0] = 3;
            lines[1] = "rep movsb"; lens[1] = 9;
            lines[2] = "cld"; lens[2] = 3;
            i.inline_asm.lines = lines;
            i.inline_asm.line_lengths = lens;
            i.inline_asm.line_count = 3;
            i.inline_asm.outputs = nullptr;
            i.inline_asm.output_count = 0;
            auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(3);
            ins[0].constraint = "D"; ins[0].vreg = dst_end;
            ins[1].constraint = "S"; ins[1].vreg = src_end;
            ins[2].constraint = "c"; ins[2].vreg = cnt;
            i.inline_asm.inputs = ins;
            i.inline_asm.input_count = 3;
            auto* clobs = ctx_.arena.makeArray<std::string_view>(3);
            clobs[0] = "rdi"; clobs[1] = "rsi"; clobs[2] = "rcx";
            i.inline_asm.clobbers = clobs;
            i.inline_asm.clobber_count = 3;
            emit(i);
        }
        emitBranch(done_bb);

        switchToBlock(done_bb);
        return INVALID_VREG;
    }

    // memcmp(a, b, count) -> i64 — compare two byte buffers
    // Returns: 0 if equal, <0 if a<b, >0 if a>b (first differing byte)
    if (callee == "memcmp" && expr->arg_count == 3) {
        VReg a_ptr = lowerExpr(expr->args[0]);
        VReg b_ptr = lowerExpr(expr->args[1]);
        VReg cnt = lowerExpr(expr->args[2]);
        VReg r = freshVReg();
        // repe cmpsb compares [rsi] vs [rdi], decrements rcx
        // After it stops: if ZF=1 (all equal) result is 0
        // else the byte at [rsi-1] - [rdi-1] gives the sign
        // We use inline asm: cld; repe cmpsb; seta al; sbb al, 0; movsx rax, al
        // This gives: 1 if a>b, -1 if a<b, 0 if equal
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = r;
        i.type = TypeTable::I64;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(5);
        auto* lens = ctx_.arena.makeArray<uint32_t>(5);
        lines[0] = "cld";              lens[0] = 3;
        lines[1] = "repe cmpsb";       lens[1] = 10;
        lines[2] = "seta al";          lens[2] = 7;
        lines[3] = "sbb al, 0";        lens[3] = 9;
        lines[4] = "movsx rax, al";    lens[4] = 13;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 5;
        auto* outs = ctx_.arena.makeArray<LIRAsmOperand>(1);
        outs[0].constraint = "=a"; outs[0].vreg = r;
        i.inline_asm.outputs = outs;
        i.inline_asm.output_count = 1;
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(3);
        ins[0].constraint = "D"; ins[0].vreg = a_ptr;   // rdi = a
        ins[1].constraint = "S"; ins[1].vreg = b_ptr;   // rsi = b
        ins[2].constraint = "c"; ins[2].vreg = cnt;     // rcx = count
        i.inline_asm.inputs = ins;
        i.inline_asm.input_count = 3;
        auto* clobs = ctx_.arena.makeArray<std::string_view>(3);
        clobs[0] = "rdi"; clobs[1] = "rsi"; clobs[2] = "rcx";
        i.inline_asm.clobbers = clobs;
        i.inline_asm.clobber_count = 3;
        emit(i);
        return r;
    }

    // memzero(dst, count) — zero-fill using rep stosb with al=0
    if (callee == "memzero" && expr->arg_count == 2) {
        VReg dst = lowerExpr(expr->args[0]);
        VReg cnt = lowerExpr(expr->args[1]);
        // Synthesize zero byte value
        VReg zero = freshVReg();
        LIRInstr zero_i{};
        zero_i.op = LIROp::ConstInt; zero_i.result = zero;
        zero_i.type = TypeTable::U8; zero_i.const_int.value = 0;
        emit(zero_i);
        // rep stosb: [rdi] = al, rdi++, rcx--
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
        ins[0].constraint = "D"; ins[0].vreg = dst;    // rdi = dst
        ins[1].constraint = "a"; ins[1].vreg = zero;   // al = 0
        ins[2].constraint = "c"; ins[2].vreg = cnt;    // rcx = count
        i.inline_asm.inputs = ins;
        i.inline_asm.input_count = 3;
        auto* clobs = ctx_.arena.makeArray<std::string_view>(3);
        clobs[0] = "rdi"; clobs[1] = "rax"; clobs[2] = "rcx";
        i.inline_asm.clobbers = clobs;
        i.inline_asm.clobber_count = 3;
        emit(i);
        return INVALID_VREG;
    }

    // strlen(ptr) -> u64 — count bytes until NUL terminator
    // Uses repnz scasb: scan for zero byte starting at rdi, rcx = max
    if (callee == "strlen" && expr->arg_count == 1) {
        VReg str = lowerExpr(expr->args[0]);
        VReg r = freshVReg();
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = r;
        i.type = TypeTable::U64;
        i.loc = expr->loc;
        // Strategy: set rcx = -1 (max), al = 0, repnz scasb,
        //           then not rcx, dec rcx → length in rcx
        auto* lines = ctx_.arena.makeArray<const char*>(5);
        auto* lens = ctx_.arena.makeArray<uint32_t>(5);
        lines[0] = "cld";            lens[0] = 3;
        lines[1] = "xor eax, eax";   lens[1] = 12;
        lines[2] = "repnz scasb";    lens[2] = 11;
        lines[3] = "not rcx";        lens[3] = 7;
        lines[4] = "dec rcx";        lens[4] = 7;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 5;
        auto* outs = ctx_.arena.makeArray<LIRAsmOperand>(1);
        outs[0].constraint = "=c"; outs[0].vreg = r;  // result in rcx
        i.inline_asm.outputs = outs;
        i.inline_asm.output_count = 1;
        // Inputs: rdi = string pointer, rcx = -1 (max search length)
        VReg max_len = freshVReg();
        LIRInstr max_i{};
        max_i.op = LIROp::ConstInt; max_i.result = max_len;
        max_i.type = TypeTable::U64; max_i.const_int.value = -1;
        emit(max_i);
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(2);
        ins[0].constraint = "D"; ins[0].vreg = str;       // rdi = string ptr
        ins[1].constraint = "c"; ins[1].vreg = max_len;   // rcx = -1
        i.inline_asm.inputs = ins;
        i.inline_asm.input_count = 2;
        auto* clobs = ctx_.arena.makeArray<std::string_view>(3);
        clobs[0] = "rdi"; clobs[1] = "rax"; clobs[2] = "rcx";
        i.inline_asm.clobbers = clobs;
        i.inline_asm.clobber_count = 3;
        emit(i);
        return r;
    }

    // strcmp(a, b) -> i64 — compare NUL-terminated strings
    // Returns 0 if equal, <0 if a<b, >0 if a>b (byte-by-byte comparison)
    if (callee == "strcmp" && expr->arg_count == 2) {
        VReg a = lowerExpr(expr->args[0]);
        VReg b = lowerExpr(expr->args[1]);
        VReg r = freshVReg();
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = r;
        i.type = TypeTable::I64;
        i.loc = expr->loc;
        // Loop-based strcmp:
        //   .loop: lodsb (load [rsi] -> al, rsi++), cmp al, [rdi],
        //          jne .done, inc rdi, test al,al, jnz .loop
        //   .done: movzx rax, byte [rdi], sub rcx, rax
        // Simpler: use repe cmpsb approach with strlen first? No, just inline asm loop.
        // Actually, simplest: manual inline asm with a loop label
        uint32_t lbl = label_counter_++;
        char buf[64];
        int n1 = snprintf(buf, sizeof(buf), ".strcmp_loop_%u:", lbl);
        auto* loop_label = static_cast<char*>(ctx_.arena.allocate(n1, 1));
        std::memcpy(loop_label, buf, n1);
        int n2 = snprintf(buf, sizeof(buf), ".strcmp_done_%u:", lbl);
        auto* done_label = static_cast<char*>(ctx_.arena.allocate(n2, 1));
        std::memcpy(done_label, buf, n2);
        char jnz_buf[64];
        int n3 = snprintf(jnz_buf, sizeof(jnz_buf), "jnz .strcmp_loop_%u", lbl);
        auto* jnz_str = static_cast<char*>(ctx_.arena.allocate(n3, 1));
        std::memcpy(jnz_str, jnz_buf, n3);
        char jne_buf[64];
        int n4 = snprintf(jne_buf, sizeof(jne_buf), "jne .strcmp_done_%u", lbl);
        auto* jne_str = static_cast<char*>(ctx_.arena.allocate(n4, 1));
        std::memcpy(jne_str, jne_buf, n4);

        auto* lines = ctx_.arena.makeArray<const char*>(10);
        auto* llens = ctx_.arena.makeArray<uint32_t>(10);
        lines[0] = "cld";                    llens[0] = 3;
        lines[1] = loop_label;               llens[1] = static_cast<uint32_t>(n1);
        lines[2] = "lodsb";                  llens[2] = 5;
        lines[3] = "cmp al, [rdi]";          llens[3] = 13;
        lines[4] = jne_str;                  llens[4] = static_cast<uint32_t>(n4);
        lines[5] = "inc rdi";                llens[5] = 7;
        lines[6] = "test al, al";            llens[6] = 11;
        lines[7] = jnz_str;                  llens[7] = static_cast<uint32_t>(n3);
        lines[8] = done_label;               llens[8] = static_cast<uint32_t>(n2);
        lines[9] = "movzx rcx, byte [rdi]\nsub rcx, rax";
                                              llens[9] = 29;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = llens;
        i.inline_asm.line_count = 10;
        auto* outs = ctx_.arena.makeArray<LIRAsmOperand>(1);
        outs[0].constraint = "=c"; outs[0].vreg = r;
        i.inline_asm.outputs = outs;
        i.inline_asm.output_count = 1;
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(2);
        ins[0].constraint = "S"; ins[0].vreg = a;  // rsi = string a
        ins[1].constraint = "D"; ins[1].vreg = b;  // rdi = string b
        i.inline_asm.inputs = ins;
        i.inline_asm.input_count = 2;
        auto* clobs = ctx_.arena.makeArray<std::string_view>(4);
        clobs[0] = "rsi"; clobs[1] = "rdi"; clobs[2] = "rax"; clobs[3] = "rcx";
        i.inline_asm.clobbers = clobs;
        i.inline_asm.clobber_count = 4;
        emit(i);
        return r;
    }

    // checked_add/checked_sub/checked_mul — trap on overflow
    if ((callee == "checked_add" || callee == "checked_sub" || callee == "checked_mul")
         && expr->arg_count == 2) {
        VReg lhs = lowerExpr(expr->args[0]);
        VReg rhs = lowerExpr(expr->args[1]);

        // Determine which LIR op to use
        LIROp op;
        if (callee == "checked_add") op = LIROp::Add;
        else if (callee == "checked_sub") op = LIROp::Sub;
        else op = LIROp::Mul;

        // Emit the normal operation
        VReg r = freshVReg();
        LIRInstr arith{};
        arith.op = op;
        arith.result = r;
        arith.type = expr->type;
        arith.loc = expr->loc;
        arith.bin.lhs = lhs;
        arith.bin.rhs = rhs;
        emit(arith);

        // Emit: compare result for overflow using a second addition/subtraction
        // and branch to trap. Use inline asm: "jo .Lpanic" pattern.
        // Since the actual overflow flag is set by the ISel add/sub instruction,
        // we need a special inline asm that just checks the flag.
        // Simpler: emit a Trap-on-overflow LIR opcode.
        // For now, use the inline asm pattern with a re-add to set flags.
        // Actually simplest: just use "into" equivalent via conditional jump.
        // Emit inline asm: test by re-doing the op with "jo" check after.
        // ... this gets complex. Let's use the approach of emitting the op
        // followed by a conditional trap via new blocks.

        // Create a "no overflow" continuation block and a trap block
        uint32_t trap_bb = newBlock(".Lchecked_trap");
        uint32_t cont_bb = newBlock(".Lchecked_ok");

        // For overflow detection: recompute with wrapping and compare.
        // Actually, the simplest correct approach at the LIR level:
        // use ICmpEq after reverse operation to detect overflow.
        // For add: if (result - rhs) != lhs → overflow
        // For sub: if (result + rhs) != lhs → overflow
        // For mul: if rhs != 0 && (result / rhs) != lhs → overflow

        if (callee == "checked_add") {
            // Check: (result - rhs) != lhs → overflow
            VReg check = freshVReg();
            LIRInstr sub{};
            sub.op = LIROp::SubWrap;
            sub.result = check;
            sub.type = expr->type;
            sub.loc = expr->loc;
            sub.bin.lhs = r;
            sub.bin.rhs = rhs;
            emit(sub);
            VReg cmp = freshVReg();
            LIRInstr eq{};
            eq.op = LIROp::ICmpEq;
            eq.result = cmp;
            eq.type = TypeTable::Bool;
            eq.loc = expr->loc;
            eq.bin.lhs = check;
            eq.bin.rhs = lhs;
            emit(eq);
            emitCondBranch(cmp, cont_bb, trap_bb);
        } else if (callee == "checked_sub") {
            // Check: (result + rhs) != lhs → overflow
            VReg check = freshVReg();
            LIRInstr add{};
            add.op = LIROp::AddWrap;
            add.result = check;
            add.type = expr->type;
            add.loc = expr->loc;
            add.bin.lhs = r;
            add.bin.rhs = rhs;
            emit(add);
            VReg cmp = freshVReg();
            LIRInstr eq{};
            eq.op = LIROp::ICmpEq;
            eq.result = cmp;
            eq.type = TypeTable::Bool;
            eq.loc = expr->loc;
            eq.bin.lhs = check;
            eq.bin.rhs = lhs;
            emit(eq);
            emitCondBranch(cmp, cont_bb, trap_bb);
        } else {
            // checked_mul: if rhs != 0 && result / rhs != lhs → overflow
            // First check if rhs == 0 (no overflow possible)
            VReg zero = freshVReg();
            LIRInstr ci{};
            ci.op = LIROp::ConstInt;
            ci.result = zero;
            ci.type = expr->type;
            ci.loc = expr->loc;
            ci.const_int.value = 0;
            emit(ci);
            VReg rhs_is_zero = freshVReg();
            LIRInstr eq0{};
            eq0.op = LIROp::ICmpEq;
            eq0.result = rhs_is_zero;
            eq0.type = TypeTable::Bool;
            eq0.loc = expr->loc;
            eq0.bin.lhs = rhs;
            eq0.bin.rhs = zero;
            emit(eq0);
            uint32_t check_bb = newBlock(".Lchecked_mul_check");
            emitCondBranch(rhs_is_zero, cont_bb, check_bb);
            // In check block: result / rhs != lhs → overflow
            switchToBlock(check_bb);
            VReg check = freshVReg();
            LIRInstr dv{};
            dv.op = LIROp::Div;
            dv.result = check;
            dv.type = expr->type;
            dv.loc = expr->loc;
            dv.bin.lhs = r;
            dv.bin.rhs = rhs;
            emit(dv);
            VReg cmp = freshVReg();
            LIRInstr eq{};
            eq.op = LIROp::ICmpEq;
            eq.result = cmp;
            eq.type = TypeTable::Bool;
            eq.loc = expr->loc;
            eq.bin.lhs = check;
            eq.bin.rhs = lhs;
            emit(eq);
            emitCondBranch(cmp, cont_bb, trap_bb);
        }

        // Trap block: emit ud2
        switchToBlock(trap_bb);
        LIRInstr trap{};
        trap.op = LIROp::Trap;
        trap.result = INVALID_VREG;
        trap.type = TypeTable::Never;
        trap.loc = expr->loc;
        emit(trap);

        // Continue block: result is available
        switchToBlock(cont_bb);
        return r;
    }

    // syscall(nr, a1..a6) -> i64
    // Uses: rax=nr, rdi/rsi/rdx/r10/r8/r9 for args. Returns rax. Clobbers rcx, r11.
    if (callee == "syscall" && expr->arg_count >= 1 && expr->arg_count <= 7) {
        uint32_t nargs = expr->arg_count;
        VReg* arg_vregs = ctx_.arena.makeArray<VReg>(nargs);
        for (uint32_t a = 0; a < nargs; ++a)
            arg_vregs[a] = lowerExpr(expr->args[a]);

        VReg r = freshVReg();
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = r;
        i.type = expr->type;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(1);
        auto* lens = ctx_.arena.makeArray<uint32_t>(1);
        lines[0] = "syscall"; lens[0] = 7;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 1;
        // Output: rax
        auto* outs = ctx_.arena.makeArray<LIRAsmOperand>(1);
        outs[0].constraint = "=a";
        outs[0].vreg = r;
        i.inline_asm.outputs = outs;
        i.inline_asm.output_count = 1;
        // Inputs: rax=nr, rdi=a1, rsi=a2, rdx=a3, r10=a4, r8=a5, r9=a6
        static constexpr const char* SYSCALL_IN_CONSTRAINTS[] = {
            "a", "D", "S", "d", "r10", "r8", "r9"
        };
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(nargs);
        for (uint32_t a = 0; a < nargs; ++a) {
            ins[a].constraint = SYSCALL_IN_CONSTRAINTS[a];
            ins[a].vreg = arg_vregs[a];
        }
        i.inline_asm.inputs = ins;
        i.inline_asm.input_count = nargs;
        // Clobbers: rcx, r11 (set by CPU), memory
        auto* clobs = ctx_.arena.makeArray<std::string_view>(3);
        clobs[0] = "rcx"; clobs[1] = "r11"; clobs[2] = "memory";
        i.inline_asm.clobbers = clobs;
        i.inline_asm.clobber_count = 3;
        emit(i);
        return r;
    }

    // save_flags() -> u64  (pushfq; pop rax)
    if (callee == "save_flags" && expr->arg_count == 0) {
        VReg r = freshVReg();
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = r;
        i.type = expr->type;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(2);
        auto* lens = ctx_.arena.makeArray<uint32_t>(2);
        lines[0] = "pushfq";  lens[0] = 6;
        lines[1] = "pop rax"; lens[1] = 7;
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

    // restore_flags(flags: u64)  (push flags; popfq)
    if (callee == "restore_flags" && expr->arg_count == 1) {
        VReg flags = lowerExpr(expr->args[0]);
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = INVALID_VREG;
        i.type = TypeTable::Unit;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(2);
        auto* lens = ctx_.arena.makeArray<uint32_t>(2);
        lines[0] = "push rdi"; lens[0] = 8;
        lines[1] = "popfq";    lens[1] = 5;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 2;
        i.inline_asm.outputs = nullptr;
        i.inline_asm.output_count = 0;
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(1);
        ins[0].constraint = "D";
        ins[0].vreg = flags;
        i.inline_asm.inputs = ins;
        i.inline_asm.input_count = 1;
        i.inline_asm.clobbers = nullptr;
        i.inline_asm.clobber_count = 0;
        emit(i);
        return INVALID_VREG;
    }

    // sidt(ptr)/sgdt(ptr) — store IDT/GDT register to memory
    if ((callee == "sidt" || callee == "sgdt") && expr->arg_count == 1) {
        VReg ptr = lowerExpr(expr->args[0]);
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = INVALID_VREG;
        i.type = TypeTable::Unit;
        i.loc = expr->loc;
        const char* asm_line = (callee == "sidt") ? "sidt [rdi]" : "sgdt [rdi]";
        uint32_t asm_len = 10;
        auto* lines = ctx_.arena.makeArray<const char*>(1);
        auto* lens = ctx_.arena.makeArray<uint32_t>(1);
        lines[0] = asm_line; lens[0] = asm_len;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 1;
        i.inline_asm.outputs = nullptr;
        i.inline_asm.output_count = 0;
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(1);
        ins[0].constraint = "D";
        ins[0].vreg = ptr;
        i.inline_asm.inputs = ins;
        i.inline_asm.input_count = 1;
        auto* clobs = ctx_.arena.makeArray<std::string_view>(1);
        clobs[0] = "memory";
        i.inline_asm.clobbers = clobs;
        i.inline_asm.clobber_count = 1;
        emit(i);
        return INVALID_VREG;
    }

    // xgetbv(xcr: u32) -> u64
    // ecx = xcr index, xgetbv writes edx:eax
    if (callee == "xgetbv" && expr->arg_count == 1) {
        VReg xcr = lowerExpr(expr->args[0]);
        VReg r = freshVReg();
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = r;
        i.type = expr->type;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(3);
        auto* lens = ctx_.arena.makeArray<uint32_t>(3);
        lines[0] = "xgetbv";          lens[0] = 6;
        lines[1] = "shl rdx, 32";     lens[1] = 11;
        lines[2] = "or rax, rdx";     lens[2] = 11;
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
        ins[0].vreg = xcr;
        i.inline_asm.inputs = ins;
        i.inline_asm.input_count = 1;
        auto* clobs = ctx_.arena.makeArray<std::string_view>(1);
        clobs[0] = "rdx";
        i.inline_asm.clobbers = clobs;
        i.inline_asm.clobber_count = 1;
        emit(i);
        return r;
    }

    // xsetbv(xcr: u32, val: u64) -> unit
    // ecx = xcr, edx:eax = val
    if (callee == "xsetbv" && expr->arg_count == 2) {
        VReg xcr = lowerExpr(expr->args[0]);
        VReg val = lowerExpr(expr->args[1]);
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = INVALID_VREG;
        i.type = TypeTable::Unit;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(3);
        auto* lens = ctx_.arena.makeArray<uint32_t>(3);
        lines[0] = "mov rdx, rax";    lens[0] = 12;
        lines[1] = "shr rdx, 32";     lens[1] = 11;
        lines[2] = "xsetbv";          lens[2] = 6;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 3;
        i.inline_asm.outputs = nullptr;
        i.inline_asm.output_count = 0;
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(2);
        ins[0].constraint = "c";
        ins[0].vreg = xcr;
        ins[1].constraint = "a";
        ins[1].vreg = val;
        i.inline_asm.inputs = ins;
        i.inline_asm.input_count = 2;
        auto* clobs = ctx_.arena.makeArray<std::string_view>(1);
        clobs[0] = "rdx";
        i.inline_asm.clobbers = clobs;
        i.inline_asm.clobber_count = 1;
        emit(i);
        return INVALID_VREG;
    }

    // monitor(addr, extensions, hints)
    // rax = addr, ecx = extensions, edx = hints
    if (callee == "monitor" && expr->arg_count == 3) {
        VReg addr = lowerExpr(expr->args[0]);
        VReg ext = lowerExpr(expr->args[1]);
        VReg hints = lowerExpr(expr->args[2]);
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = INVALID_VREG;
        i.type = TypeTable::Unit;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(1);
        auto* lens = ctx_.arena.makeArray<uint32_t>(1);
        lines[0] = "monitor"; lens[0] = 7;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 1;
        i.inline_asm.outputs = nullptr;
        i.inline_asm.output_count = 0;
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(3);
        ins[0].constraint = "a"; ins[0].vreg = addr;
        ins[1].constraint = "c"; ins[1].vreg = ext;
        ins[2].constraint = "d"; ins[2].vreg = hints;
        i.inline_asm.inputs = ins;
        i.inline_asm.input_count = 3;
        i.inline_asm.clobbers = nullptr;
        i.inline_asm.clobber_count = 0;
        emit(i);
        return INVALID_VREG;
    }

    // mwait(extensions, hints)
    // ecx = extensions, eax = hints
    if (callee == "mwait" && expr->arg_count == 2) {
        VReg ext = lowerExpr(expr->args[0]);
        VReg hints = lowerExpr(expr->args[1]);
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = INVALID_VREG;
        i.type = TypeTable::Unit;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(1);
        auto* lens = ctx_.arena.makeArray<uint32_t>(1);
        lines[0] = "mwait"; lens[0] = 5;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 1;
        i.inline_asm.outputs = nullptr;
        i.inline_asm.output_count = 0;
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(2);
        ins[0].constraint = "c"; ins[0].vreg = ext;
        ins[1].constraint = "a"; ins[1].vreg = hints;
        i.inline_asm.inputs = ins;
        i.inline_asm.input_count = 2;
        i.inline_asm.clobbers = nullptr;
        i.inline_asm.clobber_count = 0;
        emit(i);
        return INVALID_VREG;
    }

    // volatile_load(ptr) -> T  (atomic load with relaxed ordering for MMIO)
    if (callee == "volatile_load" && expr->arg_count == 1) {
        VReg ptr = lowerExpr(expr->args[0]);
        VReg r = freshVReg();
        // Use AtomicLoad with Relaxed ordering — prevents optimization/reordering
        LIRInstr i{};
        i.op = LIROp::AtomicLoad;
        i.result = r;
        i.type = expr->type;
        i.loc = expr->loc;
        i.atomic_load.ptr = ptr;
        i.atomic_load.order = MemOrder::Relaxed;
        emit(i);
        return r;
    }

    // volatile_store(ptr, val) -> unit  (atomic store with relaxed ordering for MMIO)
    if (callee == "volatile_store" && expr->arg_count == 2) {
        VReg ptr = lowerExpr(expr->args[0]);
        VReg val = lowerExpr(expr->args[1]);
        // Use AtomicStore with Relaxed ordering — prevents optimization/reordering
        LIRInstr i{};
        i.op = LIROp::AtomicStore;
        i.result = INVALID_VREG;
        i.type = TypeTable::Unit;
        i.loc = expr->loc;
        i.atomic_store.ptr = ptr;
        i.atomic_store.value = val;
        i.atomic_store.order = MemOrder::Relaxed;
        emit(i);
        return INVALID_VREG;
    }

    // min(a, b) -> T  (branchless min via cmp + cmov)
    if (callee == "min" && expr->arg_count == 2) {
        VReg a = lowerExpr(expr->args[0]);
        VReg b = lowerExpr(expr->args[1]);
        // Compare a < b, if true result = a, else result = b
        VReg cmp = freshVReg();
        LIRInstr c{};
        c.op = LIROp::ICmpLt;
        c.result = cmp;
        c.type = TypeTable::Bool;
        c.loc = expr->loc;
        c.bin.lhs = a;
        c.bin.rhs = b;
        emit(c);
        // Use conditional pattern: if a < b then a else b
        uint32_t then_bb = newBlock(".Lmin_then");
        uint32_t else_bb = newBlock(".Lmin_else");
        uint32_t merge_bb = newBlock(".Lmin_merge");
        blocks_[merge_bb].param_types.push_back(expr->type);
        emitCondBranch(cmp, then_bb, else_bb);
        switchToBlock(then_bb);
        emitBranchWithArgs(merge_bb, {a});
        switchToBlock(else_bb);
        emitBranchWithArgs(merge_bb, {b});
        switchToBlock(merge_bb);
        VReg r = freshVReg();
        LIRInstr ba{};
        ba.op = LIROp::BlockArg;
        ba.result = r;
        ba.type = expr->type;
        ba.loc = expr->loc;
        ba.block_arg.index = 0;
        emit(ba);
        return r;
    }

    // max(a, b) -> T  (branchless max via cmp + cmov)
    if (callee == "max" && expr->arg_count == 2) {
        VReg a = lowerExpr(expr->args[0]);
        VReg b = lowerExpr(expr->args[1]);
        VReg cmp = freshVReg();
        LIRInstr c{};
        c.op = LIROp::ICmpGt;
        c.result = cmp;
        c.type = TypeTable::Bool;
        c.loc = expr->loc;
        c.bin.lhs = a;
        c.bin.rhs = b;
        emit(c);
        uint32_t then_bb = newBlock(".Lmax_then");
        uint32_t else_bb = newBlock(".Lmax_else");
        uint32_t merge_bb = newBlock(".Lmax_merge");
        blocks_[merge_bb].param_types.push_back(expr->type);
        emitCondBranch(cmp, then_bb, else_bb);
        switchToBlock(then_bb);
        emitBranchWithArgs(merge_bb, {a});
        switchToBlock(else_bb);
        emitBranchWithArgs(merge_bb, {b});
        switchToBlock(merge_bb);
        VReg r = freshVReg();
        LIRInstr ba{};
        ba.op = LIROp::BlockArg;
        ba.result = r;
        ba.type = expr->type;
        ba.loc = expr->loc;
        ba.block_arg.index = 0;
        emit(ba);
        return r;
    }

    // abs(x) -> T  (branchless absolute value: x >= 0 ? x : -x)
    if (callee == "abs" && expr->arg_count == 1) {
        VReg x = lowerExpr(expr->args[0]);
        // Zero constant
        VReg zero = freshVReg();
        LIRInstr ci{};
        ci.op = LIROp::ConstInt;
        ci.result = zero;
        ci.type = expr->type;
        ci.loc = expr->loc;
        ci.const_int.value = 0;
        emit(ci);
        // Check x >= 0
        VReg cmp = freshVReg();
        LIRInstr c{};
        c.op = LIROp::ICmpGe;
        c.result = cmp;
        c.type = TypeTable::Bool;
        c.loc = expr->loc;
        c.bin.lhs = x;
        c.bin.rhs = zero;
        emit(c);
        // Negate x
        VReg neg = freshVReg();
        LIRInstr n{};
        n.op = LIROp::Neg;
        n.result = neg;
        n.type = expr->type;
        n.loc = expr->loc;
        n.unary.operand = x;
        emit(n);
        // Conditional select: x >= 0 ? x : -x
        uint32_t then_bb = newBlock(".Labs_pos");
        uint32_t else_bb = newBlock(".Labs_neg");
        uint32_t merge_bb = newBlock(".Labs_merge");
        blocks_[merge_bb].param_types.push_back(expr->type);
        emitCondBranch(cmp, then_bb, else_bb);
        switchToBlock(then_bb);
        emitBranchWithArgs(merge_bb, {x});
        switchToBlock(else_bb);
        emitBranchWithArgs(merge_bb, {neg});
        switchToBlock(merge_bb);
        VReg r = freshVReg();
        LIRInstr ba{};
        ba.op = LIROp::BlockArg;
        ba.result = r;
        ba.type = expr->type;
        ba.loc = expr->loc;
        ba.block_arg.index = 0;
        emit(ba);
        return r;
    }

    // rdrand() -> u64
    if (callee == "rdrand" && expr->arg_count == 0) {
        VReg r = freshVReg();
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = r;
        i.type = expr->type;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(1);
        auto* lens = ctx_.arena.makeArray<uint32_t>(1);
        lines[0] = "rdrand rax"; lens[0] = 10;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 1;
        auto* outs = ctx_.arena.makeArray<LIRAsmOperand>(1);
        outs[0].constraint = "=a"; outs[0].vreg = r;
        i.inline_asm.outputs = outs;
        i.inline_asm.output_count = 1;
        i.inline_asm.inputs = nullptr; i.inline_asm.input_count = 0;
        i.inline_asm.clobbers = nullptr; i.inline_asm.clobber_count = 0;
        emit(i);
        return r;
    }

    // rdseed() -> u64
    if (callee == "rdseed" && expr->arg_count == 0) {
        VReg r = freshVReg();
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = r;
        i.type = expr->type;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(1);
        auto* lens = ctx_.arena.makeArray<uint32_t>(1);
        lines[0] = "rdseed rax"; lens[0] = 10;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 1;
        auto* outs = ctx_.arena.makeArray<LIRAsmOperand>(1);
        outs[0].constraint = "=a"; outs[0].vreg = r;
        i.inline_asm.outputs = outs;
        i.inline_asm.output_count = 1;
        i.inline_asm.inputs = nullptr; i.inline_asm.input_count = 0;
        i.inline_asm.clobbers = nullptr; i.inline_asm.clobber_count = 0;
        emit(i);
        return r;
    }

    // Cache prefetch: prefetch_t0/t1/t2/nta(ptr)
    if ((callee == "prefetch_t0" || callee == "prefetch_t1" ||
         callee == "prefetch_t2" || callee == "prefetch_nta") &&
        expr->arg_count == 1) {
        VReg ptr = lowerExpr(expr->args[0]);
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = INVALID_VREG;
        i.type = TypeTable::Unit;
        i.loc = expr->loc;
        const char* asm_line;
        uint32_t asm_len;
        if (callee == "prefetch_t0")  { asm_line = "prefetcht0 [rdi]";  asm_len = 16; }
        else if (callee == "prefetch_t1") { asm_line = "prefetcht1 [rdi]";  asm_len = 16; }
        else if (callee == "prefetch_t2") { asm_line = "prefetcht2 [rdi]";  asm_len = 16; }
        else { asm_line = "prefetchnta [rdi]"; asm_len = 17; }
        auto* lines = ctx_.arena.makeArray<const char*>(1);
        auto* lens = ctx_.arena.makeArray<uint32_t>(1);
        lines[0] = asm_line; lens[0] = asm_len;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 1;
        i.inline_asm.outputs = nullptr; i.inline_asm.output_count = 0;
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(1);
        ins[0].constraint = "D"; ins[0].vreg = ptr;
        i.inline_asm.inputs = ins; i.inline_asm.input_count = 1;
        i.inline_asm.clobbers = nullptr; i.inline_asm.clobber_count = 0;
        emit(i);
        return INVALID_VREG;
    }

    // Cache line flush: clflush/clflushopt/clwb(addr)
    if ((callee == "clflush" || callee == "clflushopt" || callee == "clwb") &&
        expr->arg_count == 1) {
        VReg addr = lowerExpr(expr->args[0]);
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = INVALID_VREG;
        i.type = TypeTable::Unit;
        i.loc = expr->loc;
        const char* asm_line;
        uint32_t asm_len;
        if (callee == "clflush")    { asm_line = "clflush [rdi]";    asm_len = 13; }
        else if (callee == "clflushopt") { asm_line = "clflushopt [rdi]"; asm_len = 16; }
        else { asm_line = "clwb [rdi]"; asm_len = 10; }
        auto* lines = ctx_.arena.makeArray<const char*>(1);
        auto* lens = ctx_.arena.makeArray<uint32_t>(1);
        lines[0] = asm_line; lens[0] = asm_len;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 1;
        i.inline_asm.outputs = nullptr; i.inline_asm.output_count = 0;
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(1);
        ins[0].constraint = "D"; ins[0].vreg = addr;
        i.inline_asm.inputs = ins; i.inline_asm.input_count = 1;
        auto* clobs = ctx_.arena.makeArray<std::string_view>(1);
        clobs[0] = "memory";
        i.inline_asm.clobbers = clobs; i.inline_asm.clobber_count = 1;
        emit(i);
        return INVALID_VREG;
    }

    // rdpid() -> u64
    if (callee == "rdpid" && expr->arg_count == 0) {
        VReg r = freshVReg();
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = r;
        i.type = expr->type;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(1);
        auto* lens = ctx_.arena.makeArray<uint32_t>(1);
        lines[0] = "rdpid rax"; lens[0] = 9;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 1;
        auto* outs = ctx_.arena.makeArray<LIRAsmOperand>(1);
        outs[0].constraint = "=a"; outs[0].vreg = r;
        i.inline_asm.outputs = outs; i.inline_asm.output_count = 1;
        i.inline_asm.inputs = nullptr; i.inline_asm.input_count = 0;
        i.inline_asm.clobbers = nullptr; i.inline_asm.clobber_count = 0;
        emit(i);
        return r;
    }

    // rdfsbase()/rdgsbase() -> u64
    if ((callee == "rdfsbase" || callee == "rdgsbase") && expr->arg_count == 0) {
        VReg r = freshVReg();
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = r;
        i.type = expr->type;
        i.loc = expr->loc;
        const char* asm_line = (callee == "rdfsbase") ? "rdfsbase rax" : "rdgsbase rax";
        uint32_t asm_len = 12;
        auto* lines = ctx_.arena.makeArray<const char*>(1);
        auto* lens = ctx_.arena.makeArray<uint32_t>(1);
        lines[0] = asm_line; lens[0] = asm_len;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 1;
        auto* outs = ctx_.arena.makeArray<LIRAsmOperand>(1);
        outs[0].constraint = "=a"; outs[0].vreg = r;
        i.inline_asm.outputs = outs; i.inline_asm.output_count = 1;
        i.inline_asm.inputs = nullptr; i.inline_asm.input_count = 0;
        i.inline_asm.clobbers = nullptr; i.inline_asm.clobber_count = 0;
        emit(i);
        return r;
    }

    // wrfsbase(val)/wrgsbase(val)
    if ((callee == "wrfsbase" || callee == "wrgsbase") && expr->arg_count == 1) {
        VReg val = lowerExpr(expr->args[0]);
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = INVALID_VREG;
        i.type = TypeTable::Unit;
        i.loc = expr->loc;
        const char* asm_line = (callee == "wrfsbase") ? "wrfsbase rdi" : "wrgsbase rdi";
        uint32_t asm_len = 12;
        auto* lines = ctx_.arena.makeArray<const char*>(1);
        auto* lens = ctx_.arena.makeArray<uint32_t>(1);
        lines[0] = asm_line; lens[0] = asm_len;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 1;
        i.inline_asm.outputs = nullptr; i.inline_asm.output_count = 0;
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(1);
        ins[0].constraint = "D"; ins[0].vreg = val;
        i.inline_asm.inputs = ins; i.inline_asm.input_count = 1;
        i.inline_asm.clobbers = nullptr; i.inline_asm.clobber_count = 0;
        emit(i);
        return INVALID_VREG;
    }

    // setjmp(buf: *u8) -> i32
    // Save callee-saved regs + rsp + return address to buf, return 0
    if (callee == "setjmp" && expr->arg_count == 1) {
        VReg buf = lowerExpr(expr->args[0]);
        VReg r = freshVReg();
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = r;
        i.type = expr->type;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(10);
        auto* lens = ctx_.arena.makeArray<uint32_t>(10);
        lines[0] = "mov [rdi], rbx";       lens[0] = 14;
        lines[1] = "mov [rdi+8], r12";      lens[1] = 15;
        lines[2] = "mov [rdi+16], r13";     lens[2] = 16;
        lines[3] = "mov [rdi+24], r14";     lens[3] = 16;
        lines[4] = "mov [rdi+32], r15";     lens[4] = 16;
        lines[5] = "mov [rdi+40], rbp";     lens[5] = 16;
        lines[6] = "mov [rdi+48], rsp";     lens[6] = 16;
        lines[7] = "mov rax, [rsp]";        lens[7] = 14;  // return address
        lines[8] = "mov [rdi+56], rax";     lens[8] = 16;
        lines[9] = "xor eax, eax";          lens[9] = 12;  // return 0
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 10;
        auto* outs = ctx_.arena.makeArray<LIRAsmOperand>(1);
        outs[0].constraint = "=a";
        outs[0].vreg = r;
        i.inline_asm.outputs = outs;
        i.inline_asm.output_count = 1;
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(1);
        ins[0].constraint = "D";
        ins[0].vreg = buf;
        i.inline_asm.inputs = ins;
        i.inline_asm.input_count = 1;
        i.inline_asm.clobbers = nullptr;
        i.inline_asm.clobber_count = 0;
        emit(i);
        return r;
    }

    // longjmp(buf: *u8, val: i32) -> never
    // Restore callee-saved regs + rsp, jump to saved return address
    if (callee == "longjmp" && expr->arg_count == 2) {
        VReg buf = lowerExpr(expr->args[0]);
        VReg val = lowerExpr(expr->args[1]);
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = INVALID_VREG;
        i.type = TypeTable::Never;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(11);
        auto* lens = ctx_.arena.makeArray<uint32_t>(11);
        // Ensure val != 0 (if val == 0, set to 1)
        lines[0] = "test esi, esi";         lens[0] = 13;
        lines[1] = "jnz .Llj_ok";           lens[1] = 12;
        lines[2] = "mov esi, 1";             lens[2] = 10;
        lines[3] = ".Llj_ok:";               lens[3] = 8;
        lines[4] = "mov rbx, [rdi]";         lens[4] = 14;
        lines[5] = "mov r12, [rdi+8]";       lens[5] = 16;
        lines[6] = "mov r13, [rdi+16]";      lens[6] = 17;
        lines[7] = "mov r14, [rdi+24]";      lens[7] = 17;
        lines[8] = "mov r15, [rdi+32]";      lens[8] = 17;
        lines[9] = "mov rbp, [rdi+40]";      lens[9] = 17;
        lines[10] = "mov rsp, [rdi+48]";     lens[10] = 17;
        // We need one more: mov rax,val then jmp to saved rip
        // ... but 11 lines. Let me restructure:
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 11;
        i.inline_asm.outputs = nullptr;
        i.inline_asm.output_count = 0;
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(2);
        ins[0].constraint = "D"; ins[0].vreg = buf;
        ins[1].constraint = "S"; ins[1].vreg = val;
        i.inline_asm.inputs = ins;
        i.inline_asm.input_count = 2;
        auto* clobs = ctx_.arena.makeArray<std::string_view>(7);
        clobs[0] = "rbx"; clobs[1] = "r12"; clobs[2] = "r13";
        clobs[3] = "r14"; clobs[4] = "r15"; clobs[5] = "rbp";
        clobs[6] = "rsp";
        i.inline_asm.clobbers = clobs;
        i.inline_asm.clobber_count = 7;
        emit(i);

        // Second asm block: set return val in eax and jmp to saved rip
        LIRInstr i2{};
        i2.op = LIROp::InlineAsm;
        i2.result = INVALID_VREG;
        i2.type = TypeTable::Never;
        i2.loc = expr->loc;
        auto* lines2 = ctx_.arena.makeArray<const char*>(3);
        auto* lens2 = ctx_.arena.makeArray<uint32_t>(3);
        lines2[0] = "mov eax, esi";          lens2[0] = 12;
        lines2[1] = "mov rcx, [rdi+56]";     lens2[1] = 18;
        lines2[2] = "jmp rcx";               lens2[2] = 7;
        i2.inline_asm.lines = lines2;
        i2.inline_asm.line_lengths = lens2;
        i2.inline_asm.line_count = 3;
        i2.inline_asm.outputs = nullptr;
        i2.inline_asm.output_count = 0;
        i2.inline_asm.inputs = nullptr;
        i2.inline_asm.input_count = 0;
        auto* clobs2 = ctx_.arena.makeArray<std::string_view>(2);
        clobs2[0] = "rax"; clobs2[1] = "rcx";
        i2.inline_asm.clobbers = clobs2;
        i2.inline_asm.clobber_count = 2;
        emit(i2);
        return INVALID_VREG;
    }

    // invpcid(type, descriptor_ptr) — process-context TLB invalidation
    if (callee == "invpcid" && expr->arg_count == 2) {
        VReg type_val = lowerExpr(expr->args[0]);
        VReg desc = lowerExpr(expr->args[1]);
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = INVALID_VREG;
        i.type = TypeTable::Unit;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(1);
        auto* lens = ctx_.arena.makeArray<uint32_t>(1);
        lines[0] = "invpcid rax, [rdi]"; lens[0] = 18;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 1;
        i.inline_asm.outputs = nullptr;
        i.inline_asm.output_count = 0;
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(2);
        ins[0].constraint = "a"; ins[0].vreg = type_val;
        ins[1].constraint = "D"; ins[1].vreg = desc;
        i.inline_asm.inputs = ins;
        i.inline_asm.input_count = 2;
        auto* clobs = ctx_.arena.makeArray<std::string_view>(1);
        clobs[0] = "memory";
        i.inline_asm.clobbers = clobs;
        i.inline_asm.clobber_count = 1;
        emit(i);
        return INVALID_VREG;
    }

    // tag_of(union_value) -> u64  (read union discriminant tag)
    if (callee == "tag_of" && expr->arg_count == 1) {
        // Union tag is at offset 0, u64 (8 bytes)
        VReg base = lowerExpr(expr->args[0]);
        // The base is a pointer to the union value in memory
        // Load the tag (first 8 bytes) from the union's address
        VReg tag = freshVReg();
        LIRInstr load{};
        load.op = LIROp::Load;
        load.result = tag;
        load.type = TypeTable::U64;
        load.loc = expr->loc;
        load.load.ptr = base;
        emit(load);
        return tag;
    }

    // indirect_branch(target: *u8) — bare jmp [reg] without call frame
    if (callee == "indirect_branch" && expr->arg_count == 1) {
        VReg target = lowerExpr(expr->args[0]);
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = INVALID_VREG;
        i.type = TypeTable::Never;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(1);
        auto* lens = ctx_.arena.makeArray<uint32_t>(1);
        lines[0] = "jmp rdi"; lens[0] = 7;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 1;
        i.inline_asm.outputs = nullptr;
        i.inline_asm.output_count = 0;
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(1);
        ins[0].constraint = "D"; ins[0].vreg = target;
        i.inline_asm.inputs = ins;
        i.inline_asm.input_count = 1;
        i.inline_asm.clobbers = nullptr;
        i.inline_asm.clobber_count = 0;
        emit(i);
        return INVALID_VREG;
    }

    // nt_store(ptr, val: u64) — non-temporal store (movnti)
    if (callee == "nt_store" && expr->arg_count == 2) {
        VReg ptr = lowerExpr(expr->args[0]);
        VReg val = lowerExpr(expr->args[1]);
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = INVALID_VREG;
        i.type = TypeTable::Unit;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(1);
        auto* lens = ctx_.arena.makeArray<uint32_t>(1);
        lines[0] = "movnti [rdi], rsi"; lens[0] = 17;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 1;
        i.inline_asm.outputs = nullptr;
        i.inline_asm.output_count = 0;
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(2);
        ins[0].constraint = "D"; ins[0].vreg = ptr;
        ins[1].constraint = "S"; ins[1].vreg = val;
        i.inline_asm.inputs = ins;
        i.inline_asm.input_count = 2;
        auto* clobs = ctx_.arena.makeArray<std::string_view>(1);
        clobs[0] = "memory";
        i.inline_asm.clobbers = clobs;
        i.inline_asm.clobber_count = 1;
        emit(i);
        return INVALID_VREG;
    }

    // rotate_left(val, count) -> T  (ROL instruction)
    if (callee == "rotate_left" && expr->arg_count == 2) {
        VReg val = lowerExpr(expr->args[0]);
        VReg cnt = lowerExpr(expr->args[1]);
        VReg r = freshVReg();
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = r;
        i.type = expr->type;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(2);
        auto* lens = ctx_.arena.makeArray<uint32_t>(2);
        lines[0] = "mov rax, rdi"; lens[0] = 12;
        lines[1] = "rol rax, cl";  lens[1] = 11;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 2;
        auto* outs = ctx_.arena.makeArray<LIRAsmOperand>(1);
        outs[0].constraint = "=a"; outs[0].vreg = r;
        i.inline_asm.outputs = outs;
        i.inline_asm.output_count = 1;
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(2);
        ins[0].constraint = "D"; ins[0].vreg = val;
        ins[1].constraint = "c"; ins[1].vreg = cnt;
        i.inline_asm.inputs = ins;
        i.inline_asm.input_count = 2;
        i.inline_asm.clobbers = nullptr;
        i.inline_asm.clobber_count = 0;
        emit(i);
        return r;
    }

    // rotate_right(val, count) -> T  (ROR instruction)
    if (callee == "rotate_right" && expr->arg_count == 2) {
        VReg val = lowerExpr(expr->args[0]);
        VReg cnt = lowerExpr(expr->args[1]);
        VReg r = freshVReg();
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = r;
        i.type = expr->type;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(2);
        auto* lens = ctx_.arena.makeArray<uint32_t>(2);
        lines[0] = "mov rax, rdi"; lens[0] = 12;
        lines[1] = "ror rax, cl";  lens[1] = 11;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 2;
        auto* outs = ctx_.arena.makeArray<LIRAsmOperand>(1);
        outs[0].constraint = "=a"; outs[0].vreg = r;
        i.inline_asm.outputs = outs;
        i.inline_asm.output_count = 1;
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(2);
        ins[0].constraint = "D"; ins[0].vreg = val;
        ins[1].constraint = "c"; ins[1].vreg = cnt;
        i.inline_asm.inputs = ins;
        i.inline_asm.input_count = 2;
        i.inline_asm.clobbers = nullptr;
        i.inline_asm.clobber_count = 0;
        emit(i);
        return r;
    }

    // byte_swap_16(val: u16) -> u16  (endian swap via rol ax, 8)
    if (callee == "byte_swap_16" && expr->arg_count == 1) {
        VReg val = lowerExpr(expr->args[0]);
        VReg r = freshVReg();
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = r;
        i.type = expr->type;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(2);
        auto* lens = ctx_.arena.makeArray<uint32_t>(2);
        lines[0] = "movzx eax, di"; lens[0] = 14;
        lines[1] = "rol ax, 8";     lens[1] = 9;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 2;
        auto* outs = ctx_.arena.makeArray<LIRAsmOperand>(1);
        outs[0].constraint = "=a"; outs[0].vreg = r;
        i.inline_asm.outputs = outs;
        i.inline_asm.output_count = 1;
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(1);
        ins[0].constraint = "D"; ins[0].vreg = val;
        i.inline_asm.inputs = ins;
        i.inline_asm.input_count = 1;
        i.inline_asm.clobbers = nullptr;
        i.inline_asm.clobber_count = 0;
        emit(i);
        return r;
    }

    // rdmsr(ecx: u32) -> u64  (read model-specific register)
    if (callee == "rdmsr" && expr->arg_count == 1) {
        VReg ecx_val = lowerExpr(expr->args[0]);
        VReg r = freshVReg();
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = r;
        i.type = TypeTable::U64;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(3);
        auto* lens = ctx_.arena.makeArray<uint32_t>(3);
        lines[0] = "rdmsr";             lens[0] = 5;
        lines[1] = "shl rdx, 32";       lens[1] = 11;
        lines[2] = "or rax, rdx";       lens[2] = 10;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 3;
        auto* outs = ctx_.arena.makeArray<LIRAsmOperand>(1);
        outs[0].constraint = "=a"; outs[0].vreg = r;
        i.inline_asm.outputs = outs;
        i.inline_asm.output_count = 1;
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(1);
        ins[0].constraint = "c"; ins[0].vreg = ecx_val;
        i.inline_asm.inputs = ins;
        i.inline_asm.input_count = 1;
        auto* clobs = ctx_.arena.makeArray<std::string_view>(1);
        clobs[0] = "rdx";
        i.inline_asm.clobbers = clobs;
        i.inline_asm.clobber_count = 1;
        emit(i);
        return r;
    }

    // wrmsr(ecx: u32, val: u64) -> unit  (write model-specific register)
    if (callee == "wrmsr" && expr->arg_count == 2) {
        VReg ecx_val = lowerExpr(expr->args[0]);
        VReg val = lowerExpr(expr->args[1]);
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = INVALID_VREG;
        i.type = TypeTable::Unit;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(3);
        auto* lens = ctx_.arena.makeArray<uint32_t>(3);
        lines[0] = "mov rax, rdi";      lens[0] = 12;
        lines[1] = "mov rdx, rax";      lens[1] = 12;
        lines[2] = "shr rdx, 32";       lens[2] = 11;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 3;
        i.inline_asm.outputs = nullptr;
        i.inline_asm.output_count = 0;
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(2);
        ins[0].constraint = "c"; ins[0].vreg = ecx_val;
        ins[1].constraint = "D"; ins[1].vreg = val;
        i.inline_asm.inputs = ins;
        i.inline_asm.input_count = 2;
        auto* clobs = ctx_.arena.makeArray<std::string_view>(2);
        clobs[0] = "rax"; clobs[1] = "rdx";
        i.inline_asm.clobbers = clobs;
        i.inline_asm.clobber_count = 2;
        // Need additional wrmsr after the setup
        emit(i);
        // Emit actual wrmsr
        LIRInstr i2{};
        i2.op = LIROp::InlineAsm;
        i2.result = INVALID_VREG;
        i2.type = TypeTable::Unit;
        i2.loc = expr->loc;
        auto* lines2 = ctx_.arena.makeArray<const char*>(1);
        auto* lens2 = ctx_.arena.makeArray<uint32_t>(1);
        lines2[0] = "wrmsr"; lens2[0] = 5;
        i2.inline_asm.lines = lines2;
        i2.inline_asm.line_lengths = lens2;
        i2.inline_asm.line_count = 1;
        i2.inline_asm.outputs = nullptr; i2.inline_asm.output_count = 0;
        i2.inline_asm.inputs = nullptr; i2.inline_asm.input_count = 0;
        i2.inline_asm.clobbers = nullptr; i2.inline_asm.clobber_count = 0;
        emit(i2);
        return INVALID_VREG;
    }

    // Control register reads: read_cr0/2/3/4/8() -> u64
    if ((callee == "read_cr0" || callee == "read_cr2" ||
         callee == "read_cr3" || callee == "read_cr4" ||
         callee == "read_cr8") && expr->arg_count == 0) {
        VReg r = freshVReg();
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = r;
        i.type = TypeTable::U64;
        i.loc = expr->loc;
        const char* asm_line = nullptr;
        uint32_t asm_len = 0;
        if (callee == "read_cr0") { asm_line = "mov rax, cr0"; asm_len = 12; }
        else if (callee == "read_cr2") { asm_line = "mov rax, cr2"; asm_len = 12; }
        else if (callee == "read_cr3") { asm_line = "mov rax, cr3"; asm_len = 12; }
        else if (callee == "read_cr8") { asm_line = "mov rax, cr8"; asm_len = 12; }
        else { asm_line = "mov rax, cr4"; asm_len = 12; }
        auto* lines = ctx_.arena.makeArray<const char*>(1);
        auto* lens = ctx_.arena.makeArray<uint32_t>(1);
        lines[0] = asm_line; lens[0] = asm_len;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 1;
        auto* outs = ctx_.arena.makeArray<LIRAsmOperand>(1);
        outs[0].constraint = "=a"; outs[0].vreg = r;
        i.inline_asm.outputs = outs;
        i.inline_asm.output_count = 1;
        i.inline_asm.inputs = nullptr;
        i.inline_asm.input_count = 0;
        i.inline_asm.clobbers = nullptr;
        i.inline_asm.clobber_count = 0;
        emit(i);
        return r;
    }

    // Control register writes: write_cr0/3/4/8(val: u64) -> unit
    if ((callee == "write_cr0" || callee == "write_cr3" || callee == "write_cr4" ||
         callee == "write_cr8") &&
        expr->arg_count == 1) {
        VReg val = lowerExpr(expr->args[0]);
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = INVALID_VREG;
        i.type = TypeTable::Unit;
        i.loc = expr->loc;
        const char* asm_line = nullptr;
        uint32_t asm_len = 0;
        if (callee == "write_cr0") { asm_line = "mov cr0, rdi"; asm_len = 12; }
        else if (callee == "write_cr3") { asm_line = "mov cr3, rdi"; asm_len = 12; }
        else if (callee == "write_cr8") { asm_line = "mov cr8, rdi"; asm_len = 12; }
        else { asm_line = "mov cr4, rdi"; asm_len = 12; }
        auto* lines = ctx_.arena.makeArray<const char*>(1);
        auto* lens = ctx_.arena.makeArray<uint32_t>(1);
        lines[0] = asm_line; lens[0] = asm_len;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 1;
        i.inline_asm.outputs = nullptr;
        i.inline_asm.output_count = 0;
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(1);
        ins[0].constraint = "D"; ins[0].vreg = val;
        i.inline_asm.inputs = ins;
        i.inline_asm.input_count = 1;
        i.inline_asm.clobbers = nullptr;
        i.inline_asm.clobber_count = 0;
        emit(i);
        return INVALID_VREG;
    }

    // read_efer() -> u64 — read IA32_EFER MSR (0xC0000080)
    if (callee == "read_efer" && expr->arg_count == 0) {
        VReg r = freshVReg();
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = r;
        i.type = TypeTable::U64;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(4);
        auto* lens = ctx_.arena.makeArray<uint32_t>(4);
        lines[0] = "mov ecx, 0xC0000080"; lens[0] = 19;
        lines[1] = "rdmsr";               lens[1] = 5;
        lines[2] = "shl rdx, 32";         lens[2] = 11;
        lines[3] = "or rax, rdx";         lens[3] = 11;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 4;
        auto* outs = ctx_.arena.makeArray<LIRAsmOperand>(1);
        outs[0].constraint = "=a"; outs[0].vreg = r;
        i.inline_asm.outputs = outs;
        i.inline_asm.output_count = 1;
        i.inline_asm.inputs = nullptr;
        i.inline_asm.input_count = 0;
        auto* clobs = ctx_.arena.makeArray<std::string_view>(2);
        clobs[0] = "rcx"; clobs[1] = "rdx";
        i.inline_asm.clobbers = clobs;
        i.inline_asm.clobber_count = 2;
        emit(i);
        return r;
    }

    // write_efer(val: u64) -> Unit — write IA32_EFER MSR (0xC0000080)
    if (callee == "write_efer" && expr->arg_count == 1) {
        VReg val = lowerExpr(expr->args[0]);
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = INVALID_VREG;
        i.type = TypeTable::Unit;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(4);
        auto* lens = ctx_.arena.makeArray<uint32_t>(4);
        lines[0] = "mov rax, $0";         lens[0] = 11;
        lines[1] = "mov rdx, rax";        lens[1] = 12;
        lines[2] = "shr rdx, 32";         lens[2] = 11;
        lines[3] = "mov ecx, 0xC0000080"; lens[3] = 19;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 4;
        i.inline_asm.outputs = nullptr;
        i.inline_asm.output_count = 0;
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(1);
        ins[0].constraint = "r"; ins[0].vreg = val;
        i.inline_asm.inputs = ins;
        i.inline_asm.input_count = 1;
        auto* clobs = ctx_.arena.makeArray<std::string_view>(3);
        clobs[0] = "rax"; clobs[1] = "rcx"; clobs[2] = "rdx";
        i.inline_asm.clobbers = clobs;
        i.inline_asm.clobber_count = 3;
        emit(i);
        // wrmsr after setting eax/edx/ecx
        LIRInstr i2{};
        i2.op = LIROp::InlineAsm;
        i2.result = INVALID_VREG;
        i2.type = TypeTable::Unit;
        i2.loc = expr->loc;
        auto* lines2 = ctx_.arena.makeArray<const char*>(1);
        auto* lens2 = ctx_.arena.makeArray<uint32_t>(1);
        lines2[0] = "wrmsr"; lens2[0] = 5;
        i2.inline_asm.lines = lines2;
        i2.inline_asm.line_lengths = lens2;
        i2.inline_asm.line_count = 1;
        i2.inline_asm.outputs = nullptr;
        i2.inline_asm.output_count = 0;
        i2.inline_asm.inputs = nullptr;
        i2.inline_asm.input_count = 0;
        i2.inline_asm.clobbers = nullptr;
        i2.inline_asm.clobber_count = 0;
        emit(i2);
        return INVALID_VREG;
    }

    // rdssp() -> u64 — read shadow stack pointer (CET)
    if (callee == "rdssp" && expr->arg_count == 0) {
        VReg r = freshVReg();
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = r;
        i.type = TypeTable::U64;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(2);
        auto* lens = ctx_.arena.makeArray<uint32_t>(2);
        lines[0] = "xor eax, eax"; lens[0] = 13;
        lines[1] = "rdsspq rax";   lens[1] = 10;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 2;
        auto* outs = ctx_.arena.makeArray<LIRAsmOperand>(1);
        outs[0].constraint = "=a"; outs[0].vreg = r;
        i.inline_asm.outputs = outs;
        i.inline_asm.output_count = 1;
        i.inline_asm.inputs = nullptr;
        i.inline_asm.input_count = 0;
        i.inline_asm.clobbers = nullptr;
        i.inline_asm.clobber_count = 0;
        emit(i);
        return r;
    }

    // wrssp(val: u64) -> Unit — write shadow stack pointer (CET)
    if (callee == "wrssp" && expr->arg_count == 1) {
        VReg val = lowerExpr(expr->args[0]);
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = INVALID_VREG;
        i.type = TypeTable::Unit;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(1);
        auto* lens = ctx_.arena.makeArray<uint32_t>(1);
        lines[0] = "wrssq [$0]"; lens[0] = 10;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 1;
        i.inline_asm.outputs = nullptr;
        i.inline_asm.output_count = 0;
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(1);
        ins[0].constraint = "r"; ins[0].vreg = val;
        i.inline_asm.inputs = ins;
        i.inline_asm.input_count = 1;
        i.inline_asm.clobbers = nullptr;
        i.inline_asm.clobber_count = 0;
        emit(i);
        return INVALID_VREG;
    }

    // rstorssp(addr: u64) -> Unit — restore saved shadow stack pointer (CET)
    if (callee == "rstorssp" && expr->arg_count == 1) {
        VReg addr = lowerExpr(expr->args[0]);
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = INVALID_VREG;
        i.type = TypeTable::Unit;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(1);
        auto* lens = ctx_.arena.makeArray<uint32_t>(1);
        lines[0] = "rstorssp [$0]"; lens[0] = 13;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 1;
        i.inline_asm.outputs = nullptr;
        i.inline_asm.output_count = 0;
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(1);
        ins[0].constraint = "r"; ins[0].vreg = addr;
        i.inline_asm.inputs = ins;
        i.inline_asm.input_count = 1;
        i.inline_asm.clobbers = nullptr;
        i.inline_asm.clobber_count = 0;
        emit(i);
        return INVALID_VREG;
    }

    // setssbsy(addr: u64) -> Unit — set shadow stack busy flag (CET)
    if (callee == "setssbsy" && expr->arg_count == 1) {
        VReg addr = lowerExpr(expr->args[0]);
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = INVALID_VREG;
        i.type = TypeTable::Unit;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(1);
        auto* lens = ctx_.arena.makeArray<uint32_t>(1);
        lines[0] = "setssbsy"; lens[0] = 8;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 1;
        i.inline_asm.outputs = nullptr;
        i.inline_asm.output_count = 0;
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(1);
        ins[0].constraint = "r"; ins[0].vreg = addr;
        i.inline_asm.inputs = ins;
        i.inline_asm.input_count = 1;
        i.inline_asm.clobbers = nullptr;
        i.inline_asm.clobber_count = 0;
        emit(i);
        return INVALID_VREG;
    }

    // rdpkru() -> u32 — read protection key rights for user pages
    if (callee == "rdpkru" && expr->arg_count == 0) {
        VReg r = freshVReg();
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = r;
        i.type = TypeTable::U32;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(2);
        auto* lens = ctx_.arena.makeArray<uint32_t>(2);
        lines[0] = "xor ecx, ecx"; lens[0] = 13;
        lines[1] = "rdpkru";       lens[1] = 6;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 2;
        auto* outs = ctx_.arena.makeArray<LIRAsmOperand>(1);
        outs[0].constraint = "=a"; outs[0].vreg = r;
        i.inline_asm.outputs = outs;
        i.inline_asm.output_count = 1;
        i.inline_asm.inputs = nullptr;
        i.inline_asm.input_count = 0;
        auto* clobs = ctx_.arena.makeArray<std::string_view>(2);
        clobs[0] = "rcx"; clobs[1] = "rdx";
        i.inline_asm.clobbers = clobs;
        i.inline_asm.clobber_count = 2;
        emit(i);
        return r;
    }

    // wrpkru(val: u32) -> Unit — write protection key rights for user pages
    if (callee == "wrpkru" && expr->arg_count == 1) {
        VReg val = lowerExpr(expr->args[0]);
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = INVALID_VREG;
        i.type = TypeTable::Unit;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(3);
        auto* lens = ctx_.arena.makeArray<uint32_t>(3);
        lines[0] = "mov eax, $0";  lens[0] = 11;
        lines[1] = "xor ecx, ecx"; lens[1] = 13;
        lines[2] = "xor edx, edx"; lens[2] = 13;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 3;
        i.inline_asm.outputs = nullptr;
        i.inline_asm.output_count = 0;
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(1);
        ins[0].constraint = "r"; ins[0].vreg = val;
        i.inline_asm.inputs = ins;
        i.inline_asm.input_count = 1;
        i.inline_asm.clobbers = nullptr;
        i.inline_asm.clobber_count = 0;
        emit(i);
        // wrpkru instruction
        LIRInstr i2{};
        i2.op = LIROp::InlineAsm;
        i2.result = INVALID_VREG;
        i2.type = TypeTable::Unit;
        i2.loc = expr->loc;
        auto* lines2 = ctx_.arena.makeArray<const char*>(1);
        auto* lens2 = ctx_.arena.makeArray<uint32_t>(1);
        lines2[0] = "wrpkru"; lens2[0] = 6;
        i2.inline_asm.lines = lines2;
        i2.inline_asm.line_lengths = lens2;
        i2.inline_asm.line_count = 1;
        i2.inline_asm.outputs = nullptr;
        i2.inline_asm.output_count = 0;
        i2.inline_asm.inputs = nullptr;
        i2.inline_asm.input_count = 0;
        i2.inline_asm.clobbers = nullptr;
        i2.inline_asm.clobber_count = 0;
        emit(i2);
        return INVALID_VREG;
    }

    // atomic_load_u32(ptr) -> u32 — 32-bit atomic load
    if (callee == "atomic_load_u32" && expr->arg_count == 1) {
        VReg ptr = lowerExpr(expr->args[0]);
        VReg r = freshVReg();
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = r;
        i.type = TypeTable::U32;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(1);
        auto* lens = ctx_.arena.makeArray<uint32_t>(1);
        lines[0] = "mov eax, [$0]"; lens[0] = 13;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 1;
        auto* outs = ctx_.arena.makeArray<LIRAsmOperand>(1);
        outs[0].constraint = "=a"; outs[0].vreg = r;
        i.inline_asm.outputs = outs;
        i.inline_asm.output_count = 1;
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(1);
        ins[0].constraint = "r"; ins[0].vreg = ptr;
        i.inline_asm.inputs = ins;
        i.inline_asm.input_count = 1;
        i.inline_asm.clobbers = nullptr;
        i.inline_asm.clobber_count = 0;
        emit(i);
        return r;
    }

    // atomic_store_u32(ptr, val) -> Unit — 32-bit atomic store
    if (callee == "atomic_store_u32" && expr->arg_count == 2) {
        VReg ptr = lowerExpr(expr->args[0]);
        VReg val = lowerExpr(expr->args[1]);
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = INVALID_VREG;
        i.type = TypeTable::Unit;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(2);
        auto* lens = ctx_.arena.makeArray<uint32_t>(2);
        lines[0] = "mov [$0], $1";  lens[0] = 11;
        lines[1] = "mfence";        lens[1] = 6;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 2;
        i.inline_asm.outputs = nullptr;
        i.inline_asm.output_count = 0;
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(2);
        ins[0].constraint = "r"; ins[0].vreg = ptr;
        ins[1].constraint = "r"; ins[1].vreg = val;
        i.inline_asm.inputs = ins;
        i.inline_asm.input_count = 2;
        i.inline_asm.clobbers = nullptr;
        i.inline_asm.clobber_count = 0;
        emit(i);
        return INVALID_VREG;
    }

    // atomic_cas_u32(ptr, expected, desired) -> u32 — 32-bit CAS
    if (callee == "atomic_cas_u32" && expr->arg_count == 3) {
        VReg ptr = lowerExpr(expr->args[0]);
        VReg expected = lowerExpr(expr->args[1]);
        VReg desired = lowerExpr(expr->args[2]);
        VReg r = freshVReg();
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = r;
        i.type = TypeTable::U32;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(2);
        auto* lens = ctx_.arena.makeArray<uint32_t>(2);
        lines[0] = "mov eax, $1";             lens[0] = 11;
        lines[1] = "lock cmpxchg [$0], $2";   lens[1] = 22;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 2;
        auto* outs = ctx_.arena.makeArray<LIRAsmOperand>(1);
        outs[0].constraint = "=a"; outs[0].vreg = r;
        i.inline_asm.outputs = outs;
        i.inline_asm.output_count = 1;
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(3);
        ins[0].constraint = "r"; ins[0].vreg = ptr;
        ins[1].constraint = "r"; ins[1].vreg = expected;
        ins[2].constraint = "r"; ins[2].vreg = desired;
        i.inline_asm.inputs = ins;
        i.inline_asm.input_count = 3;
        i.inline_asm.clobbers = nullptr;
        i.inline_asm.clobber_count = 0;
        emit(i);
        return r;
    }

    // atomic_fetch_add_u32(ptr, val) -> u32 — 32-bit atomic fetch-add
    if (callee == "atomic_fetch_add_u32" && expr->arg_count == 2) {
        VReg ptr = lowerExpr(expr->args[0]);
        VReg val = lowerExpr(expr->args[1]);
        VReg r = freshVReg();
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = r;
        i.type = TypeTable::U32;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(2);
        auto* lens = ctx_.arena.makeArray<uint32_t>(2);
        lines[0] = "mov eax, $1";           lens[0] = 11;
        lines[1] = "lock xadd [$0], eax";   lens[1] = 20;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 2;
        auto* outs = ctx_.arena.makeArray<LIRAsmOperand>(1);
        outs[0].constraint = "=a"; outs[0].vreg = r;
        i.inline_asm.outputs = outs;
        i.inline_asm.output_count = 1;
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(2);
        ins[0].constraint = "r"; ins[0].vreg = ptr;
        ins[1].constraint = "r"; ins[1].vreg = val;
        i.inline_asm.inputs = ins;
        i.inline_asm.input_count = 2;
        i.inline_asm.clobbers = nullptr;
        i.inline_asm.clobber_count = 0;
        emit(i);
        return r;
    }

    // test_and_set(ptr, bit_num) -> bool — atomic BTS
    if (callee == "test_and_set" && expr->arg_count == 2) {
        VReg ptr = lowerExpr(expr->args[0]);
        VReg bit = lowerExpr(expr->args[1]);
        VReg r = freshVReg();
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = r;
        i.type = TypeTable::Bool;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(2);
        auto* lens = ctx_.arena.makeArray<uint32_t>(2);
        lines[0] = "lock bts [$0], $1"; lens[0] = 18;
        lines[1] = "setc al";           lens[1] = 7;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 2;
        auto* outs = ctx_.arena.makeArray<LIRAsmOperand>(1);
        outs[0].constraint = "=a"; outs[0].vreg = r;
        i.inline_asm.outputs = outs;
        i.inline_asm.output_count = 1;
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(2);
        ins[0].constraint = "r"; ins[0].vreg = ptr;
        ins[1].constraint = "r"; ins[1].vreg = bit;
        i.inline_asm.inputs = ins;
        i.inline_asm.input_count = 2;
        i.inline_asm.clobbers = nullptr;
        i.inline_asm.clobber_count = 0;
        emit(i);
        return r;
    }

    // test_and_clear(ptr, bit_num) -> bool — atomic BTR
    if (callee == "test_and_clear" && expr->arg_count == 2) {
        VReg ptr = lowerExpr(expr->args[0]);
        VReg bit = lowerExpr(expr->args[1]);
        VReg r = freshVReg();
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = r;
        i.type = TypeTable::Bool;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(2);
        auto* lens = ctx_.arena.makeArray<uint32_t>(2);
        lines[0] = "lock btr [$0], $1"; lens[0] = 18;
        lines[1] = "setc al";           lens[1] = 7;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 2;
        auto* outs = ctx_.arena.makeArray<LIRAsmOperand>(1);
        outs[0].constraint = "=a"; outs[0].vreg = r;
        i.inline_asm.outputs = outs;
        i.inline_asm.output_count = 1;
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(2);
        ins[0].constraint = "r"; ins[0].vreg = ptr;
        ins[1].constraint = "r"; ins[1].vreg = bit;
        i.inline_asm.inputs = ins;
        i.inline_asm.input_count = 2;
        i.inline_asm.clobbers = nullptr;
        i.inline_asm.clobber_count = 0;
        emit(i);
        return r;
    }

    // strnlen(s, max_len) -> u64 — bounded string length
    if (callee == "strnlen" && expr->arg_count == 2) {
        VReg s = lowerExpr(expr->args[0]);
        VReg max_len = lowerExpr(expr->args[1]);
        VReg r = freshVReg();
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = r;
        i.type = TypeTable::U64;
        i.loc = expr->loc;
        // repne scasb loop: rdi=s, rcx=max_len, al=0
        auto* lines = ctx_.arena.makeArray<const char*>(5);
        auto* lens = ctx_.arena.makeArray<uint32_t>(5);
        lines[0] = "mov rdi, $0";    lens[0] = 12;
        lines[1] = "mov rcx, $1";    lens[1] = 12;
        lines[2] = "xor eax, eax";   lens[2] = 13;
        lines[3] = "repne scasb";    lens[3] = 11;
        lines[4] = "sub $1, rcx";    lens[4] = 11;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 5;
        auto* outs = ctx_.arena.makeArray<LIRAsmOperand>(1);
        outs[0].constraint = "=S"; outs[0].vreg = r;
        i.inline_asm.outputs = outs;
        i.inline_asm.output_count = 1;
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(2);
        ins[0].constraint = "r"; ins[0].vreg = s;
        ins[1].constraint = "r"; ins[1].vreg = max_len;
        i.inline_asm.inputs = ins;
        i.inline_asm.input_count = 2;
        auto* clobs = ctx_.arena.makeArray<std::string_view>(3);
        clobs[0] = "rax"; clobs[1] = "rcx"; clobs[2] = "rdi";
        i.inline_asm.clobbers = clobs;
        i.inline_asm.clobber_count = 3;
        emit(i);
        return r;
    }

    // hlt/cli/sti/pause/swapgs/wbinvd — single-instruction intrinsics
    if ((callee == "hlt" || callee == "cli" || callee == "sti" ||
         callee == "pause" || callee == "swapgs" || callee == "wbinvd") && expr->arg_count == 0) {
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = INVALID_VREG;
        i.type = TypeTable::Unit;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(1);
        auto* lens = ctx_.arena.makeArray<uint32_t>(1);
        if (callee == "hlt") { lines[0] = "hlt"; lens[0] = 3; }
        else if (callee == "cli") { lines[0] = "cli"; lens[0] = 3; }
        else if (callee == "sti") { lines[0] = "sti"; lens[0] = 3; }
        else if (callee == "pause") { lines[0] = "pause"; lens[0] = 5; }
        else if (callee == "swapgs") { lines[0] = "swapgs"; lens[0] = 6; }
        else { lines[0] = "wbinvd"; lens[0] = 6; }
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 1;
        i.inline_asm.outputs = nullptr; i.inline_asm.output_count = 0;
        i.inline_asm.inputs = nullptr; i.inline_asm.input_count = 0;
        i.inline_asm.clobbers = nullptr; i.inline_asm.clobber_count = 0;
        emit(i);
        return INVALID_VREG;
    }

    // VMX: vmxon(addr: Ptr<u8>) -> Unit
    if (callee == "vmxon" && expr->arg_count == 1) {
        VReg addr = lowerExpr(expr->args[0]);
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = INVALID_VREG;
        i.type = TypeTable::Unit;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(1);
        auto* lens = ctx_.arena.makeArray<uint32_t>(1);
        lines[0] = "vmxon [$0]"; lens[0] = 10;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 1;
        i.inline_asm.outputs = nullptr; i.inline_asm.output_count = 0;
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(1);
        ins[0].constraint = "r"; ins[0].vreg = addr;
        i.inline_asm.inputs = ins;
        i.inline_asm.input_count = 1;
        i.inline_asm.clobbers = nullptr; i.inline_asm.clobber_count = 0;
        emit(i);
        return INVALID_VREG;
    }

    // VMX: vmxoff/vmcall/vmlaunch/vmresume — single-instruction
    if ((callee == "vmxoff" || callee == "vmcall" ||
         callee == "vmlaunch" || callee == "vmresume") && expr->arg_count == 0) {
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = INVALID_VREG;
        i.type = TypeTable::Unit;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(1);
        auto* lens = ctx_.arena.makeArray<uint32_t>(1);
        if (callee == "vmxoff") { lines[0] = "vmxoff"; lens[0] = 6; }
        else if (callee == "vmcall") { lines[0] = "vmcall"; lens[0] = 6; }
        else if (callee == "vmlaunch") { lines[0] = "vmlaunch"; lens[0] = 8; }
        else { lines[0] = "vmresume"; lens[0] = 8; }
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 1;
        i.inline_asm.outputs = nullptr; i.inline_asm.output_count = 0;
        i.inline_asm.inputs = nullptr; i.inline_asm.input_count = 0;
        i.inline_asm.clobbers = nullptr; i.inline_asm.clobber_count = 0;
        emit(i);
        return INVALID_VREG;
    }

    // Port I/O: inb(port: u16) -> u8
    if (callee == "inb" && expr->arg_count == 1) {
        VReg port = lowerExpr(expr->args[0]);
        VReg r = freshVReg();
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = r;
        i.type = TypeTable::U8;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(1);
        auto* lens = ctx_.arena.makeArray<uint32_t>(1);
        lines[0] = "in al, dx"; lens[0] = 10;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 1;
        auto* outs = ctx_.arena.makeArray<LIRAsmOperand>(1);
        outs[0].constraint = "=a"; outs[0].vreg = r;
        i.inline_asm.outputs = outs;
        i.inline_asm.output_count = 1;
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(1);
        ins[0].constraint = "d"; ins[0].vreg = port;
        i.inline_asm.inputs = ins;
        i.inline_asm.input_count = 1;
        i.inline_asm.clobbers = nullptr;
        i.inline_asm.clobber_count = 0;
        emit(i);
        return r;
    }

    // Port I/O: inw(port: u16) -> u16
    if (callee == "inw" && expr->arg_count == 1) {
        VReg port = lowerExpr(expr->args[0]);
        VReg r = freshVReg();
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = r;
        i.type = TypeTable::U16;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(1);
        auto* lens = ctx_.arena.makeArray<uint32_t>(1);
        lines[0] = "in ax, dx"; lens[0] = 9;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 1;
        auto* outs = ctx_.arena.makeArray<LIRAsmOperand>(1);
        outs[0].constraint = "=a"; outs[0].vreg = r;
        i.inline_asm.outputs = outs;
        i.inline_asm.output_count = 1;
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(1);
        ins[0].constraint = "d"; ins[0].vreg = port;
        i.inline_asm.inputs = ins;
        i.inline_asm.input_count = 1;
        i.inline_asm.clobbers = nullptr;
        i.inline_asm.clobber_count = 0;
        emit(i);
        return r;
    }

    // Port I/O: ind(port: u16) -> u32
    if (callee == "ind" && expr->arg_count == 1) {
        VReg port = lowerExpr(expr->args[0]);
        VReg r = freshVReg();
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = r;
        i.type = TypeTable::U32;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(1);
        auto* lens = ctx_.arena.makeArray<uint32_t>(1);
        lines[0] = "in eax, dx"; lens[0] = 10;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 1;
        auto* outs = ctx_.arena.makeArray<LIRAsmOperand>(1);
        outs[0].constraint = "=a"; outs[0].vreg = r;
        i.inline_asm.outputs = outs;
        i.inline_asm.output_count = 1;
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(1);
        ins[0].constraint = "d"; ins[0].vreg = port;
        i.inline_asm.inputs = ins;
        i.inline_asm.input_count = 1;
        i.inline_asm.clobbers = nullptr;
        i.inline_asm.clobber_count = 0;
        emit(i);
        return r;
    }

    // Port I/O: outb(port: u16, val: u8) -> unit
    if (callee == "outb" && expr->arg_count == 2) {
        VReg port = lowerExpr(expr->args[0]);
        VReg val = lowerExpr(expr->args[1]);
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = INVALID_VREG;
        i.type = TypeTable::Unit;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(1);
        auto* lens = ctx_.arena.makeArray<uint32_t>(1);
        lines[0] = "out dx, al"; lens[0] = 10;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 1;
        i.inline_asm.outputs = nullptr;
        i.inline_asm.output_count = 0;
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(2);
        ins[0].constraint = "d"; ins[0].vreg = port;
        ins[1].constraint = "a"; ins[1].vreg = val;
        i.inline_asm.inputs = ins;
        i.inline_asm.input_count = 2;
        i.inline_asm.clobbers = nullptr;
        i.inline_asm.clobber_count = 0;
        emit(i);
        return INVALID_VREG;
    }

    // Port I/O: outw(port: u16, val: u16) -> unit
    if (callee == "outw" && expr->arg_count == 2) {
        VReg port = lowerExpr(expr->args[0]);
        VReg val = lowerExpr(expr->args[1]);
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = INVALID_VREG;
        i.type = TypeTable::Unit;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(1);
        auto* lens = ctx_.arena.makeArray<uint32_t>(1);
        lines[0] = "out dx, ax"; lens[0] = 10;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 1;
        i.inline_asm.outputs = nullptr;
        i.inline_asm.output_count = 0;
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(2);
        ins[0].constraint = "d"; ins[0].vreg = port;
        ins[1].constraint = "a"; ins[1].vreg = val;
        i.inline_asm.inputs = ins;
        i.inline_asm.input_count = 2;
        i.inline_asm.clobbers = nullptr;
        i.inline_asm.clobber_count = 0;
        emit(i);
        return INVALID_VREG;
    }

    // Port I/O: outd(port: u16, val: u32) -> unit
    if (callee == "outd" && expr->arg_count == 2) {
        VReg port = lowerExpr(expr->args[0]);
        VReg val = lowerExpr(expr->args[1]);
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = INVALID_VREG;
        i.type = TypeTable::Unit;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(1);
        auto* lens = ctx_.arena.makeArray<uint32_t>(1);
        lines[0] = "out dx, eax"; lens[0] = 11;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 1;
        i.inline_asm.outputs = nullptr;
        i.inline_asm.output_count = 0;
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(2);
        ins[0].constraint = "d"; ins[0].vreg = port;
        ins[1].constraint = "a"; ins[1].vreg = val;
        i.inline_asm.inputs = ins;
        i.inline_asm.input_count = 2;
        i.inline_asm.clobbers = nullptr;
        i.inline_asm.clobber_count = 0;
        emit(i);
        return INVALID_VREG;
    }

    // cpuid_eax(leaf) -> u32  (returns eax after cpuid)
    if (callee == "cpuid_eax" && expr->arg_count == 1) {
        VReg leaf = lowerExpr(expr->args[0]);
        VReg r = freshVReg();
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = r;
        i.type = TypeTable::U32;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(2);
        auto* lens = ctx_.arena.makeArray<uint32_t>(2);
        lines[0] = "xor ecx, ecx"; lens[0] = 13;
        lines[1] = "cpuid";        lens[1] = 5;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 2;
        auto* outs = ctx_.arena.makeArray<LIRAsmOperand>(1);
        outs[0].constraint = "=a"; outs[0].vreg = r;
        i.inline_asm.outputs = outs;
        i.inline_asm.output_count = 1;
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(1);
        ins[0].constraint = "a"; ins[0].vreg = leaf;
        i.inline_asm.inputs = ins;
        i.inline_asm.input_count = 1;
        auto* clobs = ctx_.arena.makeArray<std::string_view>(3);
        clobs[0] = "rbx"; clobs[1] = "rcx"; clobs[2] = "rdx";
        i.inline_asm.clobbers = clobs;
        i.inline_asm.clobber_count = 3;
        emit(i);
        return r;
    }

    // cpuid_ebx(leaf) -> u32  (returns ebx after cpuid)
    if (callee == "cpuid_ebx" && expr->arg_count == 1) {
        VReg leaf = lowerExpr(expr->args[0]);
        VReg r = freshVReg();
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = r;
        i.type = TypeTable::U32;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(3);
        auto* lens = ctx_.arena.makeArray<uint32_t>(3);
        lines[0] = "xor ecx, ecx";   lens[0] = 13;
        lines[1] = "cpuid";          lens[1] = 5;
        lines[2] = "mov eax, ebx";   lens[2] = 12;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 3;
        auto* outs = ctx_.arena.makeArray<LIRAsmOperand>(1);
        outs[0].constraint = "=a"; outs[0].vreg = r;
        i.inline_asm.outputs = outs;
        i.inline_asm.output_count = 1;
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(1);
        ins[0].constraint = "a"; ins[0].vreg = leaf;
        i.inline_asm.inputs = ins;
        i.inline_asm.input_count = 1;
        auto* clobs = ctx_.arena.makeArray<std::string_view>(3);
        clobs[0] = "rbx"; clobs[1] = "rcx"; clobs[2] = "rdx";
        i.inline_asm.clobbers = clobs;
        i.inline_asm.clobber_count = 3;
        emit(i);
        return r;
    }

    // cpuid_ecx(leaf) -> u32  (returns ecx after cpuid)
    if (callee == "cpuid_ecx" && expr->arg_count == 1) {
        VReg leaf = lowerExpr(expr->args[0]);
        VReg r = freshVReg();
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = r;
        i.type = TypeTable::U32;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(3);
        auto* lens = ctx_.arena.makeArray<uint32_t>(3);
        lines[0] = "xor ecx, ecx";   lens[0] = 13;
        lines[1] = "cpuid";          lens[1] = 5;
        lines[2] = "mov eax, ecx";   lens[2] = 12;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 3;
        auto* outs = ctx_.arena.makeArray<LIRAsmOperand>(1);
        outs[0].constraint = "=a"; outs[0].vreg = r;
        i.inline_asm.outputs = outs;
        i.inline_asm.output_count = 1;
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(1);
        ins[0].constraint = "a"; ins[0].vreg = leaf;
        i.inline_asm.inputs = ins;
        i.inline_asm.input_count = 1;
        auto* clobs = ctx_.arena.makeArray<std::string_view>(3);
        clobs[0] = "rbx"; clobs[1] = "rcx"; clobs[2] = "rdx";
        i.inline_asm.clobbers = clobs;
        i.inline_asm.clobber_count = 3;
        emit(i);
        return r;
    }

    // cpuid_edx(leaf) -> u32  (returns edx after cpuid)
    if (callee == "cpuid_edx" && expr->arg_count == 1) {
        VReg leaf = lowerExpr(expr->args[0]);
        VReg r = freshVReg();
        LIRInstr i{};
        i.op = LIROp::InlineAsm;
        i.result = r;
        i.type = TypeTable::U32;
        i.loc = expr->loc;
        auto* lines = ctx_.arena.makeArray<const char*>(3);
        auto* lens = ctx_.arena.makeArray<uint32_t>(3);
        lines[0] = "xor ecx, ecx";   lens[0] = 13;
        lines[1] = "cpuid";          lens[1] = 5;
        lines[2] = "mov eax, edx";   lens[2] = 12;
        i.inline_asm.lines = lines;
        i.inline_asm.line_lengths = lens;
        i.inline_asm.line_count = 3;
        auto* outs = ctx_.arena.makeArray<LIRAsmOperand>(1);
        outs[0].constraint = "=a"; outs[0].vreg = r;
        i.inline_asm.outputs = outs;
        i.inline_asm.output_count = 1;
        auto* ins = ctx_.arena.makeArray<LIRAsmOperand>(1);
        ins[0].constraint = "a"; ins[0].vreg = leaf;
        i.inline_asm.inputs = ins;
        i.inline_asm.input_count = 1;
        auto* clobs = ctx_.arena.makeArray<std::string_view>(3);
        clobs[0] = "rbx"; clobs[1] = "rcx"; clobs[2] = "rdx";
        i.inline_asm.clobbers = clobs;
        i.inline_asm.clobber_count = 3;
        emit(i);
        return r;
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

    // ================================================================
    // Jump table optimization: detect dense integer match without guards
    // ================================================================
    if (expr->arm_count >= 4 && type_info.kind == TypeKind::Primitive) {
        bool all_int = true;
        bool has_wildcard = false;
        uint32_t wildcard_idx = 0;
        int64_t min_val = INT64_MAX, max_val = INT64_MIN;

        for (uint32_t a = 0; a < expr->arm_count; ++a) {
            auto& arm = expr->arms[a];
            if (arm.guard) { all_int = false; break; }
            if (arm.pattern->kind == HIRPattern::Kind::IntLit) {
                int64_t v = static_cast<const HIRIntLitPattern*>(arm.pattern)->value;
                if (v < min_val) min_val = v;
                if (v > max_val) max_val = v;
            } else if (arm.pattern->kind == HIRPattern::Kind::Wildcard ||
                       arm.pattern->kind == HIRPattern::Kind::Variable) {
                has_wildcard = true;
                wildcard_idx = a;
            } else {
                all_int = false; break;
            }
        }

        // Check density: range should be at most 4x the number of cases
        uint32_t int_count = expr->arm_count - (has_wildcard ? 1 : 0);
        int64_t range = max_val - min_val + 1;
        bool dense = all_int && has_wildcard && int_count >= 3 &&
                     range > 0 && range <= static_cast<int64_t>(int_count) * 4;

        if (dense) {
            // Build arm body blocks
            uint32_t default_bb = newBlock("switch_default");
            auto* cases = ctx_.arena.makeArray<LIRSwitchCase>(int_count);
            uint32_t ci = 0;
            std::vector<std::pair<uint32_t, uint32_t>> arm_bodies; // (arm_idx, body_bb)

            for (uint32_t a = 0; a < expr->arm_count; ++a) {
                if (a == wildcard_idx) continue;
                auto* pat = static_cast<const HIRIntLitPattern*>(expr->arms[a].pattern);
                uint32_t body_bb = newBlock("switch_arm");
                cases[ci].value = pat->value;
                cases[ci].target_block = body_bb;
                arm_bodies.push_back({a, body_bb});
                ++ci;
            }

            // Emit Switch instruction
            LIRInstr sw{};
            sw.op = LIROp::Switch;
            sw.result = INVALID_VREG;
            sw.type = TypeTable::Unit;
            sw.switch_.scrutinee = scrutinee;
            sw.switch_.default_block = default_bb;
            sw.switch_.cases = cases;
            sw.switch_.case_count = int_count;
            sw.switch_.min_value = min_val;
            sw.switch_.max_value = max_val;
            sw.loc = expr->loc;
            emit(sw);

            // Emit arm bodies
            for (auto& [arm_idx, body_bb] : arm_bodies) {
                switchToBlock(body_bb);
                VReg arm_val = lowerExpr(expr->arms[arm_idx].body);
                if (!blockTerminated(blocks_[current_block_].instrs)) {
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
            }

            // Emit default arm body
            switchToBlock(default_bb);
            VReg def_val = lowerExpr(expr->arms[wildcard_idx].body);
            if (!blockTerminated(blocks_[current_block_].instrs)) {
                if (slot != INVALID_VREG && def_val != INVALID_VREG) {
                    LIRInstr st{};
                    st.op = LIROp::Store;
                    st.result = INVALID_VREG;
                    st.type = TypeTable::Unit;
                    st.store.ptr = slot;
                    st.store.value = def_val;
                    emit(st);
                }
                emitBranch(merge_bb);
            }

            // Merge
            switchToBlock(merge_bb);
            if (slot != INVALID_VREG) {
                VReg result = freshVReg();
                LIRInstr ld{};
                ld.op = LIROp::Load;
                ld.result = result;
                ld.type = expr->type;
                ld.load.ptr = slot;
                emit(ld);
                return result;
            }
            return INVALID_VREG;
        }
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
            case HIRPattern::Kind::Range: {
                auto* pat = static_cast<const HIRRangePattern*>(arm.pattern);
                // Emit: scrutinee >= lo && scrutinee <= hi (inclusive)
                //    or: scrutinee >= lo && scrutinee < hi  (exclusive)
                VReg lo_val = freshVReg();
                LIRInstr lo_i{};
                lo_i.op = LIROp::ConstInt;
                lo_i.result = lo_val;
                lo_i.type = scrut_type;
                lo_i.const_int.value = pat->lo;
                emit(lo_i);

                VReg hi_val = freshVReg();
                LIRInstr hi_i{};
                hi_i.op = LIROp::ConstInt;
                hi_i.result = hi_val;
                hi_i.type = scrut_type;
                hi_i.const_int.value = pat->hi;
                emit(hi_i);

                VReg ge_cmp = freshVReg();
                LIRInstr ge_i{};
                ge_i.op = LIROp::ICmpGe;
                ge_i.result = ge_cmp;
                ge_i.type = TypeTable::Bool;
                ge_i.bin.lhs = scrutinee;
                ge_i.bin.rhs = lo_val;
                emit(ge_i);

                VReg hi_cmp = freshVReg();
                LIRInstr hi_ci{};
                hi_ci.op = pat->inclusive ? LIROp::ICmpLe : LIROp::ICmpLt;
                hi_ci.result = hi_cmp;
                hi_ci.type = TypeTable::Bool;
                hi_ci.bin.lhs = scrutinee;
                hi_ci.bin.rhs = hi_val;
                emit(hi_ci);

                // AND the two conditions (bitwise AND on bools = logical AND)
                VReg in_range = freshVReg();
                LIRInstr and_i{};
                and_i.op = LIROp::BAnd;
                and_i.result = in_range;
                and_i.type = TypeTable::Bool;
                and_i.bin.lhs = ge_cmp;
                and_i.bin.rhs = hi_cmp;
                emit(and_i);

                emitCondBranch(in_range, body_bb, next_bb);
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
            // Check if this field is an aggregate (struct/union) that needs inline copy
            bool is_aggregate = false;
            uint32_t field_size = 0;
            if (field_type < ctx_.types.size()) {
                const auto& fti = ctx_.types.get(field_type);
                if (fti.kind == TypeKind::Struct || fti.kind == TypeKind::Union) {
                    is_aggregate = true;
                    field_size = ctx_.types.sizeOf(field_type);
                }
            }

            if (is_aggregate && field_size > 0) {
                // Inline copy: copy aggregate data 8 bytes at a time
                uint32_t aligned = (field_size + 7u) & ~7u;
                for (uint32_t off = 0; off < aligned; off += 8) {
                    VReg src_ptr = freshVReg();
                    LIRInstr sp{};
                    sp.op = LIROp::FieldPtr;
                    sp.result = src_ptr;
                    sp.type = ctx_.types.makePtr(TypeTable::I64, false);
                    sp.field_ptr.base = val;
                    sp.field_ptr.offset = off;
                    sp.loc = expr->fields[f].loc;
                    emit(sp);

                    VReg tmp = freshVReg();
                    LIRInstr ld{};
                    ld.op = LIROp::Load;
                    ld.result = tmp;
                    ld.type = TypeTable::I64;
                    ld.load.ptr = src_ptr;
                    emit(ld);

                    VReg dst_ptr = freshVReg();
                    LIRInstr dp{};
                    dp.op = LIROp::FieldPtr;
                    dp.result = dst_ptr;
                    dp.type = ctx_.types.makePtr(TypeTable::I64, false);
                    dp.field_ptr.base = base;
                    dp.field_ptr.offset = offset + off;
                    dp.loc = expr->fields[f].loc;
                    emit(dp);

                    LIRInstr st{};
                    st.op = LIROp::Store;
                    st.result = INVALID_VREG;
                    st.type = TypeTable::Unit;
                    st.store.ptr = dst_ptr;
                    st.store.value = tmp;
                    emit(st);
                }
            } else {
                // Scalar field: direct store
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
                store.type = field_type;
                store.store.ptr = fp;
                store.store.value = val;
                emit(store);
            }
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
        // For val struct/array bindings, locals_ holds the data address directly.
        // Emit FieldPtr +0 to get a Ptr-typed vreg (not struct-tagged).
        auto val_it = locals_.find(ident->name);
        if (val_it != locals_.end()) {
            TypeId operand_type = expr->operand->type;
            if (operand_type < ctx_.types.size()) {
                const auto& ti = ctx_.types.get(operand_type);
                if (ti.kind == TypeKind::Struct || ti.kind == TypeKind::Array) {
                    VReg ptr = freshVReg();
                    LIRInstr fp{};
                    fp.op = LIROp::FieldPtr;
                    fp.result = ptr;
                    fp.type = expr->type; // Ptr type
                    fp.field_ptr.base = val_it->second;
                    fp.field_ptr.offset = 0;
                    fp.loc = expr->loc;
                    emit(fp);
                    return ptr;
                }
            }
        }
    }

    // For &arr[i], compute the element pointer directly without loading the value
    if (expr->operand->kind == HIRExpr::Kind::IndexAccess) {
        auto* idx_expr = static_cast<const HIRIndexAccessExpr*>(expr->operand);
        return lowerIndexElementPtr(idx_expr);
    }

    // For &var global_name, emit LeaGlobal to get address of the global
    // (rather than loading the value and wrapping it in AddrOf)
    if (expr->operand->kind == HIRExpr::Kind::Ident) {
        auto* ident = static_cast<const HIRIdentExpr*>(expr->operand);
        auto gv_it = global_label_map_.find(ident->name);
        if (gv_it != global_label_map_.end()) {
            VReg r = freshVReg();
            LIRInstr i{};
            i.op = LIROp::LoadGlobal;
            i.result = r;
            // Use an array type to force the backend to emit LeaGlobal
            // (address) instead of MovLoadGlobal (value).
            i.type = ctx_.types.makeArrayType(TypeTable::U8, 1);
            auto nasm_it = global_nasm_label_.find(ident->name);
            i.load_global.label = (nasm_it != global_nasm_label_.end()) ? nasm_it->second : ident->name;
            i.loc = expr->loc;
            emit(i);
            return r;
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

    // Register labeled loop target
    if (!expr->label.empty()) {
        labeled_loops_[expr->label] = {header_bb, exit_bb};
    }

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
    if (!expr->label.empty()) {
        labeled_loops_.erase(expr->label);
    }
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
    // Resolve target: labeled or innermost loop
    uint32_t target_exit = current_loop_exit_;
    if (!expr->label.empty()) {
        auto it = labeled_loops_.find(expr->label);
        if (it != labeled_loops_.end()) {
            target_exit = it->second.exit_bb;
        }
    }
    emitBranchWithArgs(target_exit, args);
    return args.empty() ? INVALID_VREG : args[0];
}

VReg LIRBuilder::lowerContinue(const HIRContinueExpr* expr) {
    // Lower continue args (new accumulator values)
    std::vector<VReg> args;
    for (uint32_t i = 0; i < expr->arg_count; ++i) {
        args.push_back(lowerExpr(expr->args[i]));
    }
    // Resolve target: labeled or innermost loop
    uint32_t target_header = current_loop_header_;
    if (!expr->label.empty()) {
        auto it = labeled_loops_.find(expr->label);
        if (it != labeled_loops_.end()) {
            target_header = it->second.header_bb;
        }
    }
    emitBranchWithArgs(target_header, args);
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
            alloc.struct_alloc.align = s->explicit_align > 0 ? s->explicit_align : 8;
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
                // Use NASM label (handles link_name for extern globals)
                auto nasm_it = global_nasm_label_.find(s->name);
                sg.store_global.label = (nasm_it != global_nasm_label_.end()) ? nasm_it->second : s->name;
                sg.store_global.value = val;
                sg.loc = s->loc;
                emit(sg);
            } else {
                auto it = var_addrs_.find(s->name);
                if (it != var_addrs_.end()) {
                    TypeId val_type = s->value->type;
                    if (isAggregate(val_type)) {
                        // var slot stores a pointer to the struct data area.
                        // Load the data pointer, then copy struct data.
                        uint32_t sz = ctx_.types.sizeOf(val_type);
                        VReg dst_data = freshVReg();
                        LIRInstr ld{};
                        ld.op = LIROp::Load;
                        ld.result = dst_data;
                        ld.type = ctx_.types.makePtr(val_type, true);
                        ld.load.ptr = it->second;
                        ld.loc = s->loc;
                        emit(ld);
                        emitStructCopy(dst_data, val, sz, s->loc);
                    } else {
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
            }
            break;
        }
        case HIRStmt::Kind::FieldAssign: {
            auto* s = static_cast<const HIRFieldAssignStmt*>(stmt);
            if (s->target->kind == HIRExpr::Kind::FieldAccess) {
                auto* fa = static_cast<const HIRFieldAccessExpr*>(s->target);
                VReg obj = lowerToAddress(fa->object);
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

                    TypeId field_type = fa->type;
                    if (isAggregate(field_type)) {
                        uint32_t sz = ctx_.types.sizeOf(field_type);
                        emitStructCopy(fp, val, sz, s->loc);
                    } else {
                        LIRInstr store{};
                        store.op = LIROp::Store;
                        store.result = INVALID_VREG;
                        store.type = field_type;
                        store.store.ptr = fp;
                        store.store.value = val;
                        store.loc = s->loc;
                        emit(store);
                    }
                }
            }
            break;
        }
        case HIRStmt::Kind::DerefAssign: {
            auto* s = static_cast<const HIRDerefAssignStmt*>(stmt);
            // Get the target address — use lowerToAddress for field chains
            VReg ptr;
            if (s->target->kind == HIRExpr::Kind::Deref) {
                auto* deref = static_cast<const HIRDerefExpr*>(s->target);
                ptr = lowerExpr(deref->operand);
            } else if (s->target->kind == HIRExpr::Kind::FieldAccess) {
                ptr = lowerToAddress(s->target);
            } else {
                ptr = lowerExpr(s->target);
            }
            VReg val = lowerExpr(s->value);
            // Determine the type being stored (unwrap pointer to get pointee type)
            TypeId val_type = s->value->type;
            if (isAggregate(val_type)) {
                uint32_t sz = ctx_.types.sizeOf(val_type);
                emitStructCopy(ptr, val, sz, s->loc);
            } else {
                LIRInstr store{};
                store.op = LIROp::Store;
                store.result = INVALID_VREG;
                store.type = val_type;
                store.store.ptr = ptr;
                store.store.value = val;
                store.loc = s->loc;
                emit(store);
            }
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

            // Store value at elem_ptr — use struct copy for aggregates
            TypeId val_type = s->value->type;
            if (isAggregate(val_type)) {
                uint32_t sz = ctx_.types.sizeOf(val_type);
                emitStructCopy(elem_ptr, val, sz, s->loc);
            } else {
                LIRInstr store{};
                store.op = LIROp::Store;
                store.result = INVALID_VREG;
                store.type = val_type;
                store.store.ptr = elem_ptr;
                store.store.value = val;
                store.loc = s->loc;
                emit(store);
            }
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
    TypeId elem_type = TypeTable::I64;
    bool elem_is_aggregate = false;
    if (arr_type < ctx_.types.size()) {
        const auto& ti = ctx_.types.get(arr_type);
        if (ti.kind == TypeKind::Array) {
            elem_type = ti.array.element;
            elem_size = ctx_.types.sizeOf(elem_type);
            const auto& eti = ctx_.types.get(elem_type);
            elem_is_aggregate = (eti.kind == TypeKind::Struct ||
                                 eti.kind == TypeKind::Array);
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

    // Store each element: base + elem_size * index
    for (uint32_t i = 0; i < expr->element_count; ++i) {
        VReg val = lowerExpr(expr->elements[i]);
        uint32_t slot_offset = elem_size * i;

        if (elem_is_aggregate) {
            // Aggregate element (struct/array): val is a pointer to the data.
            // Copy elem_size bytes from val to the array slot inline.
            uint32_t aligned_size = (elem_size + 7u) & ~7u;
            for (uint32_t off = 0; off < aligned_size; off += 8) {
                // Load 8 bytes from source struct
                VReg src_ptr = freshVReg();
                LIRInstr src_fp{};
                src_fp.op = LIROp::FieldPtr;
                src_fp.result = src_ptr;
                src_fp.type = TypeTable::I64;
                src_fp.field_ptr.base = val;
                src_fp.field_ptr.offset = off;
                src_fp.loc = expr->loc;
                emit(src_fp);

                VReg tmp = freshVReg();
                LIRInstr load{};
                load.op = LIROp::Load;
                load.result = tmp;
                load.type = TypeTable::I64;
                load.load.ptr = src_ptr;
                load.loc = expr->loc;
                emit(load);

                // Store 8 bytes to destination array slot
                VReg dst_ptr = freshVReg();
                LIRInstr dst_fp{};
                dst_fp.op = LIROp::FieldPtr;
                dst_fp.result = dst_ptr;
                dst_fp.type = TypeTable::I64;
                dst_fp.field_ptr.base = base;
                dst_fp.field_ptr.offset = slot_offset + off;
                dst_fp.loc = expr->loc;
                emit(dst_fp);

                LIRInstr store{};
                store.op = LIROp::Store;
                store.result = INVALID_VREG;
                store.type = TypeTable::Unit;
                store.store.ptr = dst_ptr;
                store.store.value = tmp;
                store.loc = expr->loc;
                emit(store);
            }
        } else {
            // Scalar element: store value directly
            VReg fp = freshVReg();
            LIRInstr fp_instr{};
            fp_instr.op = LIROp::FieldPtr;
            fp_instr.result = fp;
            fp_instr.type = ctx_.types.makePtr(elem_type, false);
            fp_instr.field_ptr.base = base;
            fp_instr.field_ptr.offset = slot_offset;
            fp_instr.loc = expr->loc;
            emit(fp_instr);

            LIRInstr store{};
            store.op = LIROp::Store;
            store.result = INVALID_VREG;
            store.type = elem_type;
            store.store.ptr = fp;
            store.store.value = val;
            store.loc = expr->loc;
            emit(store);
        }
    }

    return base;
}

VReg LIRBuilder::lowerIndexElementPtr(const HIRIndexAccessExpr* expr) {
    VReg base = lowerExpr(expr->array);
    VReg idx = lowerExpr(expr->index);

    // Get element type and size from array or pointer type
    TypeId arr_type = expr->array->type;
    uint32_t elem_size = 8; // default
    uint32_t array_size = 0; // known array size (0 = unknown/pointer)
    if (arr_type < ctx_.types.size()) {
        const auto& ti = ctx_.types.get(arr_type);
        if (ti.kind == TypeKind::Array) {
            elem_size = ctx_.types.sizeOf(ti.array.element);
            array_size = ti.array.count;
        } else if (ti.kind == TypeKind::Ptr || ti.kind == TypeKind::PtrMut) {
            elem_size = ctx_.types.sizeOf(ti.ptr.pointee);
        }
    }

    // Runtime bounds check for fixed-size arrays (when index is not compile-time constant)
    if (array_size > 0 && bounds_check_) {
        // Emit: if (idx >= array_size) { ud2 }
        VReg sz = freshVReg();
        LIRInstr sz_instr{};
        sz_instr.op = LIROp::ConstInt;
        sz_instr.result = sz;
        sz_instr.type = TypeTable::I64;
        sz_instr.const_int.value = static_cast<int64_t>(array_size);
        emit(sz_instr);

        // cmp = (idx >= array_size) — unsigned comparison
        VReg cmp = freshVReg();
        LIRInstr cmp_instr{};
        cmp_instr.op = LIROp::ICmpGe;
        cmp_instr.result = cmp;
        cmp_instr.type = TypeTable::Bool;
        cmp_instr.bin.lhs = idx;
        cmp_instr.bin.rhs = sz;
        cmp_instr.loc = expr->loc;
        emit(cmp_instr);

        uint32_t trap_bb = newBlock("bounds_trap");
        uint32_t ok_bb = newBlock("bounds_ok");

        LIRInstr cbr{};
        cbr.op = LIROp::CondBranch;
        cbr.type = TypeTable::Unit;
        cbr.cond_branch.cond = cmp;
        cbr.cond_branch.true_target = trap_bb;
        cbr.cond_branch.false_target = ok_bb;
        cbr.loc = expr->loc;
        emit(cbr);

        // Trap block: ud2
        switchToBlock(trap_bb);
        LIRInstr trap{};
        trap.op = LIROp::Trap;
        trap.type = TypeTable::Unit;
        trap.loc = expr->loc;
        emit(trap);

        // Continue in ok block
        switchToBlock(ok_bb);
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
