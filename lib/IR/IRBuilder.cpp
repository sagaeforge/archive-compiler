#include "kern/ir/IRBuilder.h"
#include <cassert>
#include <string>

namespace kern {

const char* irOpcodeName(IROpcode op) {
    switch (op) {
        case IROpcode::ConstInt:   return "const_int";
        case IROpcode::ConstFloat: return "const_float";
        case IROpcode::Add:        return "add";
        case IROpcode::Sub:        return "sub";
        case IROpcode::Mul:        return "mul";
        case IROpcode::Div:        return "div";
        case IROpcode::FAdd:       return "fadd";
        case IROpcode::FSub:       return "fsub";
        case IROpcode::FMul:       return "fmul";
        case IROpcode::FDiv:       return "fdiv";
        case IROpcode::ICmpEq:     return "icmp_eq";
        case IROpcode::ICmpNe:     return "icmp_ne";
        case IROpcode::ICmpLt:     return "icmp_lt";
        case IROpcode::ICmpLe:     return "icmp_le";
        case IROpcode::ICmpGt:     return "icmp_gt";
        case IROpcode::ICmpGe:     return "icmp_ge";
        case IROpcode::FCmpEq:     return "fcmp_eq";
        case IROpcode::FCmpNe:     return "fcmp_ne";
        case IROpcode::FCmpLt:     return "fcmp_lt";
        case IROpcode::FCmpLe:     return "fcmp_le";
        case IROpcode::FCmpGt:     return "fcmp_gt";
        case IROpcode::FCmpGe:     return "fcmp_ge";
        case IROpcode::Neg:        return "neg";
        case IROpcode::FNeg:       return "fneg";
        case IROpcode::Not:        return "not";
        case IROpcode::Branch:     return "br";
        case IROpcode::CondBranch: return "condbr";
        case IROpcode::Ret:        return "ret";
        case IROpcode::Call:        return "call";
        case IROpcode::StructAlloc: return "struct_alloc";
        case IROpcode::FieldStore:  return "field_store";
        case IROpcode::FieldLoad:   return "field_load";
    }
    return "?";
}

void dumpIR(const IRModule& mod, std::ostream& out) {
    for (const auto& fn : mod.functions) {
        out << "fn " << fn.name;
        if (fn.meta.purity != Purity::Unknown) {
            out << " [" << purityName(fn.meta.purity) << "]";
        }
        out << "(";
        for (size_t i = 0; i < fn.param_values.size(); ++i) {
            if (i > 0) out << ", ";
            out << "%" << fn.param_names[i] << " = %" << fn.param_values[i];
            if (i < fn.param_types.size()) {
                out << " : " << irTypeName(fn.param_types[i]);
            }
        }
        out << ") -> " << irTypeName(fn.return_type) << " {\n";

        for (size_t bi = 0; bi < fn.blocks.size(); ++bi) {
            const auto& block = fn.blocks[bi];
            out << "  " << block.label << ":";
            if (!block.params.empty()) {
                out << "(";
                for (size_t pi = 0; pi < block.params.size(); ++pi) {
                    if (pi > 0) out << ", ";
                    out << "%" << block.params[pi];
                }
                out << ")";
            }
            out << "\n";

            for (const auto& instr : block.instrs) {
                out << "    ";
                if (instr.result != INVALID_VALUE) {
                    out << "%" << instr.result;
                    if (instr.type != IRType::Unknown) {
                        out << ":" << irTypeName(instr.type);
                    }
                    out << " = ";
                }
                if (instr.op == IROpcode::Call && instr.is_tail_call) {
                    out << "tail ";
                }
                out << irOpcodeName(instr.op);

                switch (instr.op) {
                    case IROpcode::ConstInt:
                        out << " " << instr.imm_value;
                        break;
                    case IROpcode::ConstFloat:
                        out << " " << instr.imm_float;
                        break;
                    case IROpcode::Call:
                        out << " @" << instr.callee_name << "(";
                        for (size_t i = 0; i < instr.operands.size(); ++i) {
                            if (i > 0) out << ", ";
                            out << "%" << instr.operands[i];
                        }
                        out << ")";
                        break;
                    case IROpcode::Branch:
                        out << " " << fn.blocks[instr.target_block].label;
                        break;
                    case IROpcode::CondBranch:
                        out << " %" << instr.operands[0]
                            << ", " << fn.blocks[instr.target_block].label
                            << ", " << fn.blocks[instr.false_block].label;
                        break;
                    case IROpcode::Ret:
                        if (!instr.operands.empty()) {
                            out << " %" << instr.operands[0];
                        }
                        break;
                    case IROpcode::StructAlloc:
                        out << " " << instr.callee_name
                            << " (size=" << instr.imm_value << ")";
                        break;
                    case IROpcode::FieldStore:
                        out << " %" << instr.operands[0]
                            << ", %" << instr.operands[1]
                            << " (offset=" << instr.imm_value << ")";
                        break;
                    case IROpcode::FieldLoad:
                        out << " %" << instr.operands[0]
                            << " (offset=" << instr.imm_value << ")";
                        break;
                    default:
                        for (size_t i = 0; i < instr.operands.size(); ++i) {
                            out << " %" << instr.operands[i];
                            if (i + 1 < instr.operands.size()) out << ",";
                        }
                        break;
                }
                out << "\n";
            }
        }
        out << "}\n\n";
    }
}

// --- IRBuilder ---

ValueId IRBuilder::newValue() {
    return current_fn_->next_value++;
}

uint32_t IRBuilder::newBlock(const std::string& label) {
    current_fn_->blocks.push_back({label, {}, {}});
    return static_cast<uint32_t>(current_fn_->blocks.size() - 1);
}

void IRBuilder::switchToBlock(uint32_t idx) {
    current_block_ = idx;
}

ValueId IRBuilder::emit(IRInstr instr) {
    ValueId result = instr.result;
    current_fn_->blocks[current_block_].instrs.push_back(std::move(instr));
    return result;
}

void IRBuilder::populateStructInfo() {
    // Copy struct layout info from TypeChecker into IR-level representation
    // We iterate Module's structs to discover all struct names
    // and query the TypeChecker for their definitions.
    // (Called from build() after tc_ is set)
}

const IRStructInfo* IRBuilder::getStructInfo(const std::string& name) const {
    auto it = struct_info_.find(name);
    return it != struct_info_.end() ? &it->second : nullptr;
}

IRModule IRBuilder::build(Module* mod, const TypeChecker& tc) {
    tc_ = &tc;
    module_.functions.clear();
    struct_info_.clear();

    // Populate struct info from TypeChecker
    for (uint32_t i = 0; i < mod->struct_count; ++i) {
        auto* sd = mod->structs[i];
        std::string name(sd->name);
        const StructDef* def = tc_->getStructDef(sd->name);
        if (!def) continue;

        IRStructInfo info;
        info.name = name;
        info.total_size = def->total_size;
        for (const auto& f : def->fields) {
            IRStructInfo::FieldInfo fi;
            fi.name = std::string(f.name);
            fi.offset = f.offset;
            fi.type = irTypeFromSemaType(f.type);
            fi.struct_name = std::string(f.struct_name);
            info.fields.push_back(std::move(fi));
        }
        struct_info_[name] = std::move(info);
    }

    for (uint32_t i = 0; i < mod->fn_count; ++i) {
        if (mod->functions[i]->is_intrinsic) continue;
        buildFunction(mod->functions[i]);
    }
    return std::move(module_);
}

void IRBuilder::buildFunction(FnDecl* fn) {
    module_.functions.push_back({});
    current_fn_ = &module_.functions.back();
    current_fn_->name = std::string(fn->name);
    current_fn_->next_value = 0;
    label_counter_ = 0;
    locals_.clear();

    // Set return type
    current_fn_->return_type = irTypeFromSemaType(tc_->typeOfExpr(fn->body));

    uint32_t entry = newBlock("entry");
    switchToBlock(entry);

    // Register parameters with types
    for (uint32_t i = 0; i < fn->param_count; ++i) {
        ValueId pv = newValue();
        current_fn_->param_values.push_back(pv);
        current_fn_->param_names.push_back(std::string(fn->params[i].name));
        auto pname = fn->params[i].type.name;
        IRType pt = IRType::Unknown;
        if (pname == "i8")        pt = IRType::I8;
        else if (pname == "i16")  pt = IRType::I16;
        else if (pname == "i32")  pt = IRType::I32;
        else if (pname == "i64")  pt = IRType::I64;
        else if (pname == "u8")   pt = IRType::U8;
        else if (pname == "u16")  pt = IRType::U16;
        else if (pname == "u32")  pt = IRType::U32;
        else if (pname == "u64")  pt = IRType::U64;
        else if (pname == "f32")  pt = IRType::F32;
        else if (pname == "f64")  pt = IRType::F64;
        else if (pname == "bool") pt = IRType::Bool;
        else if (struct_info_.count(std::string(pname))) pt = IRType::Struct;
        current_fn_->param_types.push_back(pt);
        locals_[fn->params[i].name] = pv;
    }

    ValueId result = buildExpr(fn->body, /*in_tail_position=*/true);

    auto& instrs = current_fn_->blocks[current_block_].instrs;
    if (instrs.empty() || instrs.back().op != IROpcode::Ret) {
        IRInstr ret;
        ret.op = IROpcode::Ret;
        ret.operands = {result};
        emit(ret);
    }
}

ValueId IRBuilder::buildExpr(Expr* expr, bool in_tail_position) {
    IRType expr_type = IRType::Unknown;
    if (tc_) {
        expr_type = irTypeFromSemaType(tc_->typeOfExpr(expr));
    }

    switch (expr->kind) {
        case Expr::Kind::IntLit: {
            auto* lit = static_cast<IntLitExpr*>(expr);
            IRInstr instr;
            instr.op = IROpcode::ConstInt;
            instr.result = newValue();
            instr.imm_value = lit->value;
            instr.loc = expr->loc;
            instr.type = expr_type;
            return emit(instr);
        }

        case Expr::Kind::FloatLit: {
            auto* fl = static_cast<FloatLitExpr*>(expr);
            IRInstr instr;
            instr.op = IROpcode::ConstFloat;
            instr.result = newValue();
            instr.imm_float = fl->value;
            instr.loc = expr->loc;
            instr.type = expr_type;
            return emit(instr);
        }

        case Expr::Kind::BoolLit: {
            auto* lit = static_cast<BoolLitExpr*>(expr);
            IRInstr instr;
            instr.op = IROpcode::ConstInt;
            instr.result = newValue();
            instr.imm_value = lit->value ? 1 : 0;
            instr.loc = expr->loc;
            instr.type = IRType::Bool;
            return emit(instr);
        }

        case Expr::Kind::Ident: {
            auto* ident = static_cast<IdentExpr*>(expr);
            auto it = locals_.find(ident->name);
            assert(it != locals_.end());
            return it->second;
        }

        case Expr::Kind::BinOp: {
            auto* bin = static_cast<BinOpExpr*>(expr);

            // Short-circuit: `a and b` → if a then b else false
            if (bin->op == BinOpKind::And) {
                ValueId lhs = buildExpr(bin->lhs);
                uint32_t rhs_block = newBlock("and_rhs_" + std::to_string(label_counter_));
                uint32_t false_block = newBlock("and_false_" + std::to_string(label_counter_));
                uint32_t merge_block = newBlock("and_merge_" + std::to_string(label_counter_));
                label_counter_++;

                IRInstr br;
                br.op = IROpcode::CondBranch;
                br.operands = {lhs};
                br.target_block = rhs_block;
                br.false_block = false_block;
                emit(br);

                switchToBlock(rhs_block);
                ValueId rhs = buildExpr(bin->rhs);
                uint32_t rhs_end = current_block_;
                IRInstr br_rhs;
                br_rhs.op = IROpcode::Branch;
                br_rhs.target_block = merge_block;
                emit(br_rhs);

                switchToBlock(false_block);
                IRInstr false_const;
                false_const.op = IROpcode::ConstInt;
                false_const.result = newValue();
                false_const.imm_value = 0;
                false_const.type = IRType::Bool;
                ValueId false_val = emit(false_const);
                IRInstr br_false;
                br_false.op = IROpcode::Branch;
                br_false.target_block = merge_block;
                emit(br_false);

                switchToBlock(merge_block);
                ValueId merge_val = newValue();
                current_fn_->blocks[merge_block].params = {
                    merge_val, rhs, false_val,
                    static_cast<ValueId>(rhs_end),
                    static_cast<ValueId>(false_block)
                };
                current_fn_->blocks[merge_block].is_merge = true;
                return merge_val;
            }

            // Short-circuit: `a or b` → if a then true else b
            if (bin->op == BinOpKind::Or) {
                ValueId lhs = buildExpr(bin->lhs);
                uint32_t true_block = newBlock("or_true_" + std::to_string(label_counter_));
                uint32_t rhs_block = newBlock("or_rhs_" + std::to_string(label_counter_));
                uint32_t merge_block = newBlock("or_merge_" + std::to_string(label_counter_));
                label_counter_++;

                IRInstr br;
                br.op = IROpcode::CondBranch;
                br.operands = {lhs};
                br.target_block = true_block;
                br.false_block = rhs_block;
                emit(br);

                switchToBlock(true_block);
                IRInstr true_const;
                true_const.op = IROpcode::ConstInt;
                true_const.result = newValue();
                true_const.imm_value = 1;
                true_const.type = IRType::Bool;
                ValueId true_val = emit(true_const);
                IRInstr br_true;
                br_true.op = IROpcode::Branch;
                br_true.target_block = merge_block;
                emit(br_true);

                switchToBlock(rhs_block);
                ValueId rhs = buildExpr(bin->rhs);
                uint32_t rhs_end = current_block_;
                IRInstr br_rhs;
                br_rhs.op = IROpcode::Branch;
                br_rhs.target_block = merge_block;
                emit(br_rhs);

                switchToBlock(merge_block);
                ValueId merge_val = newValue();
                current_fn_->blocks[merge_block].params = {
                    merge_val, true_val, rhs,
                    static_cast<ValueId>(true_block),
                    static_cast<ValueId>(rhs_end)
                };
                current_fn_->blocks[merge_block].is_merge = true;
                return merge_val;
            }

            ValueId lhs = buildExpr(bin->lhs);
            ValueId rhs = buildExpr(bin->rhs);

            bool is_float = irTypeIsFloat(expr_type) ||
                            (tc_ && irTypeIsFloat(irTypeFromSemaType(tc_->typeOfExpr(bin->lhs))));

            IROpcode op;
            if (is_float) {
                switch (bin->op) {
                    case BinOpKind::Add:   op = IROpcode::FAdd; break;
                    case BinOpKind::Sub:   op = IROpcode::FSub; break;
                    case BinOpKind::Mul:   op = IROpcode::FMul; break;
                    case BinOpKind::Div:   op = IROpcode::FDiv; break;
                    case BinOpKind::Eq:    op = IROpcode::FCmpEq; break;
                    case BinOpKind::NotEq: op = IROpcode::FCmpNe; break;
                    case BinOpKind::Lt:    op = IROpcode::FCmpLt; break;
                    case BinOpKind::LtEq:  op = IROpcode::FCmpLe; break;
                    case BinOpKind::Gt:    op = IROpcode::FCmpGt; break;
                    case BinOpKind::GtEq:  op = IROpcode::FCmpGe; break;
                    default: op = IROpcode::FAdd; break;
                }
            } else {
                switch (bin->op) {
                    case BinOpKind::Add:   op = IROpcode::Add; break;
                    case BinOpKind::Sub:   op = IROpcode::Sub; break;
                    case BinOpKind::Mul:   op = IROpcode::Mul; break;
                    case BinOpKind::Div:   op = IROpcode::Div; break;
                    case BinOpKind::Eq:    op = IROpcode::ICmpEq; break;
                    case BinOpKind::NotEq: op = IROpcode::ICmpNe; break;
                    case BinOpKind::Lt:    op = IROpcode::ICmpLt; break;
                    case BinOpKind::LtEq:  op = IROpcode::ICmpLe; break;
                    case BinOpKind::Gt:    op = IROpcode::ICmpGt; break;
                    case BinOpKind::GtEq:  op = IROpcode::ICmpGe; break;
                    default: op = IROpcode::Add; break;
                }
            }

            IRInstr instr;
            instr.op = op;
            instr.result = newValue();
            instr.operands = {lhs, rhs};
            instr.loc = expr->loc;
            instr.type = expr_type;
            return emit(instr);
        }

        case Expr::Kind::UnaryOp: {
            auto* unary = static_cast<UnaryOpExpr*>(expr);
            ValueId operand = buildExpr(unary->operand);

            if (unary->op == UnaryOpKind_t::Neg) {
                IRInstr neg;
                neg.op = irTypeIsFloat(expr_type) ? IROpcode::FNeg : IROpcode::Neg;
                neg.result = newValue();
                neg.operands = {operand};
                neg.loc = expr->loc;
                neg.type = expr_type;
                return emit(neg);
            } else if (unary->op == UnaryOpKind_t::Not) {
                IRInstr notInstr;
                notInstr.op = IROpcode::Not;
                notInstr.result = newValue();
                notInstr.operands = {operand};
                notInstr.loc = expr->loc;
                notInstr.type = IRType::Bool;
                return emit(notInstr);
            }
            return operand;
        }

        case Expr::Kind::Call: {
            auto* call = static_cast<CallExpr*>(expr);
            std::vector<ValueId> args;
            for (uint32_t i = 0; i < call->arg_count; ++i) {
                args.push_back(buildExpr(call->args[i]));
            }

            IRInstr instr;
            instr.op = IROpcode::Call;
            instr.result = newValue();
            instr.callee_name = std::string(call->callee);
            instr.operands = std::move(args);
            instr.loc = expr->loc;
            instr.type = expr_type;
            instr.is_tail_call = in_tail_position;
            return emit(instr);
        }

        case Expr::Kind::If: {
            auto* ifE = static_cast<IfExpr*>(expr);
            ValueId cond = buildExpr(ifE->condition);

            uint32_t then_block = newBlock("then_" + std::to_string(label_counter_));
            uint32_t else_block = newBlock("else_" + std::to_string(label_counter_));
            uint32_t merge_block = newBlock("merge_" + std::to_string(label_counter_));
            label_counter_++;

            IRInstr br;
            br.op = IROpcode::CondBranch;
            br.operands = {cond};
            br.target_block = then_block;
            br.false_block = else_block;
            emit(br);

            switchToBlock(then_block);
            ValueId then_val = buildExpr(ifE->then_branch, in_tail_position);
            uint32_t then_end_block = current_block_;

            IRInstr br_then;
            br_then.op = IROpcode::Branch;
            br_then.target_block = merge_block;
            emit(br_then);

            switchToBlock(else_block);
            ValueId else_val;
            if (ifE->else_branch) {
                else_val = buildExpr(ifE->else_branch, in_tail_position);
            } else {
                IRInstr unit;
                unit.op = IROpcode::ConstInt;
                unit.result = newValue();
                unit.imm_value = 0;
                unit.type = IRType::Unit;
                else_val = emit(unit);
            }
            uint32_t else_end_block = current_block_;

            IRInstr br_else;
            br_else.op = IROpcode::Branch;
            br_else.target_block = merge_block;
            emit(br_else);

            switchToBlock(merge_block);

            ValueId merge_val = newValue();
            current_fn_->blocks[merge_block].params = {
                merge_val, then_val, else_val,
                static_cast<ValueId>(then_end_block),
                static_cast<ValueId>(else_end_block)
            };
            current_fn_->blocks[merge_block].is_merge = true;

            return merge_val;
        }

        case Expr::Kind::Block: {
            auto* block = static_cast<BlockExpr*>(expr);
            for (uint32_t i = 0; i < block->stmt_count; ++i) {
                buildStmt(block->stmts[i]);
            }
            if (block->result) {
                return buildExpr(block->result, in_tail_position);
            }
            IRInstr unit;
            unit.op = IROpcode::ConstInt;
            unit.result = newValue();
            unit.imm_value = 0;
            unit.type = IRType::Unit;
            return emit(unit);
        }

        case Expr::Kind::Return: {
            auto* ret = static_cast<ReturnExpr*>(expr);
            ValueId val;
            if (ret->value) {
                val = buildExpr(ret->value, true);
            } else {
                IRInstr unit;
                unit.op = IROpcode::ConstInt;
                unit.result = newValue();
                unit.imm_value = 0;
                unit.type = IRType::Unit;
                val = emit(unit);
            }
            IRInstr retInstr;
            retInstr.op = IROpcode::Ret;
            retInstr.operands = {val};
            emit(retInstr);
            return val;
        }

        case Expr::Kind::StructLit: {
            auto* slit = static_cast<StructLitExpr*>(expr);
            std::string sname(slit->struct_name);
            auto* info = getStructInfo(sname);
            assert(info && "struct info not found");

            // StructAlloc: allocate stack space
            IRInstr alloc;
            alloc.op = IROpcode::StructAlloc;
            alloc.result = newValue();
            alloc.imm_value = info->total_size;
            alloc.callee_name = sname;
            alloc.loc = expr->loc;
            alloc.type = IRType::Struct;
            ValueId base = emit(alloc);

            // FieldStore for each field init
            for (uint32_t i = 0; i < slit->field_count; ++i) {
                auto& fi = slit->fields[i];
                ValueId val = buildExpr(fi.value);

                // Find field offset from struct info
                int32_t offset = 0;
                IRType ftype = IRType::Unknown;
                for (const auto& finfo : info->fields) {
                    if (finfo.name == fi.name) {
                        offset = finfo.offset;
                        ftype = finfo.type;
                        break;
                    }
                }

                IRInstr store;
                store.op = IROpcode::FieldStore;
                store.operands = {base, val};
                store.imm_value = offset;
                store.loc = fi.loc;
                store.type = ftype;
                emit(store);
            }

            return base;
        }

        case Expr::Kind::FieldAccess: {
            auto* fa = static_cast<FieldAccessExpr*>(expr);
            ValueId obj = buildExpr(fa->object);

            // Resolve struct name from object expression
            std::string_view sname_sv = tc_->structNameOfExpr(fa->object);
            if (sname_sv.empty()) {
                // Try local struct name for ident
                if (fa->object->kind == Expr::Kind::Ident) {
                    auto* ident = static_cast<IdentExpr*>(fa->object);
                    sname_sv = tc_->localStructName(ident->name);
                }
            }
            std::string sname(sname_sv);
            auto* info = getStructInfo(sname);
            assert(info && "struct info not found for field access");

            int32_t offset = 0;
            IRType ftype = IRType::Unknown;
            std::string nested_struct_name;
            for (const auto& finfo : info->fields) {
                if (finfo.name == fa->field_name) {
                    offset = finfo.offset;
                    ftype = finfo.type;
                    nested_struct_name = finfo.struct_name;
                    break;
                }
            }

            IRInstr load;
            load.op = IROpcode::FieldLoad;
            load.result = newValue();
            load.operands = {obj};
            load.imm_value = offset;
            load.loc = expr->loc;
            load.type = ftype;
            if (!nested_struct_name.empty()) {
                load.callee_name = nested_struct_name;
                // Encode nested struct size in target_block for CodeGen
                auto* nested_info = getStructInfo(nested_struct_name);
                if (nested_info) {
                    load.target_block = static_cast<uint32_t>(nested_info->total_size);
                }
            }
            return emit(load);
        }

        case Expr::Kind::Match: {
            auto* matchE = static_cast<MatchExpr*>(expr);
            ValueId scrutinee = buildExpr(matchE->scrutinee);
            return buildMatchChain(matchE, scrutinee, 0, in_tail_position);
        }
    }

    IRInstr dummy;
    dummy.op = IROpcode::ConstInt;
    dummy.result = newValue();
    dummy.imm_value = 0;
    return emit(dummy);
}

ValueId IRBuilder::buildMatchChain(MatchExpr* matchE, ValueId scrutinee,
                                    uint32_t arm_idx, bool in_tail_position) {
    auto& arm = matchE->arms[arm_idx];
    bool is_last = (arm_idx + 1 >= matchE->arm_count);

    // Check if this arm is unconditional (wildcard/variable with no guard)
    bool unconditional = false;
    if (arm.pattern->kind == Pattern::Kind::Wildcard && !arm.guard) {
        unconditional = true;
    }
    if (arm.pattern->kind == Pattern::Kind::Variable && !arm.guard) {
        unconditional = true;
    }

    if (unconditional || is_last) {
        // Variable binding: map variable name to scrutinee value
        if (arm.pattern->kind == Pattern::Kind::Variable) {
            auto* vp = static_cast<VariablePattern*>(arm.pattern);
            locals_[vp->name] = scrutinee;
        }
        return buildExpr(arm.body, in_tail_position);
    }

    // Build condition: compare scrutinee to pattern
    ValueId cond = INVALID_VALUE;

    IRType scrut_type = IRType::Unknown;
    if (tc_) {
        scrut_type = irTypeFromSemaType(tc_->typeOfExpr(matchE->scrutinee));
    }

    switch (arm.pattern->kind) {
        case Pattern::Kind::IntLit: {
            auto* ip = static_cast<IntLitPattern*>(arm.pattern);
            IRInstr pat_const;
            pat_const.op = IROpcode::ConstInt;
            pat_const.result = newValue();
            pat_const.imm_value = ip->value;
            pat_const.type = scrut_type;
            ValueId pat_val = emit(pat_const);

            IRInstr cmp;
            cmp.op = IROpcode::ICmpEq;
            cmp.result = newValue();
            cmp.operands = {scrutinee, pat_val};
            cmp.type = IRType::Bool;
            cond = emit(cmp);
            break;
        }
        case Pattern::Kind::BoolLit: {
            auto* bp = static_cast<BoolLitPattern*>(arm.pattern);
            IRInstr pat_const;
            pat_const.op = IROpcode::ConstInt;
            pat_const.result = newValue();
            pat_const.imm_value = bp->value ? 1 : 0;
            pat_const.type = IRType::Bool;
            ValueId pat_val = emit(pat_const);

            IRInstr cmp;
            cmp.op = IROpcode::ICmpEq;
            cmp.result = newValue();
            cmp.operands = {scrutinee, pat_val};
            cmp.type = IRType::Bool;
            cond = emit(cmp);
            break;
        }
        case Pattern::Kind::Variable: {
            // Variable with guard — bind and evaluate guard
            auto* vp = static_cast<VariablePattern*>(arm.pattern);
            locals_[vp->name] = scrutinee;
            cond = buildExpr(arm.guard);
            break;
        }
        case Pattern::Kind::Wildcard: {
            // Wildcard with guard — evaluate guard
            cond = buildExpr(arm.guard);
            break;
        }
    }

    // CondBranch: if cond, arm body, else next arm chain
    uint32_t then_block = newBlock("match_arm_" + std::to_string(label_counter_));
    uint32_t else_block = newBlock("match_next_" + std::to_string(label_counter_));
    uint32_t merge_block = newBlock("match_merge_" + std::to_string(label_counter_));
    label_counter_++;

    IRInstr br;
    br.op = IROpcode::CondBranch;
    br.operands = {cond};
    br.target_block = then_block;
    br.false_block = else_block;
    emit(br);

    // Then: build arm body
    switchToBlock(then_block);
    if (arm.pattern->kind == Pattern::Kind::Variable) {
        auto* vp = static_cast<VariablePattern*>(arm.pattern);
        locals_[vp->name] = scrutinee;
    }
    ValueId then_val = buildExpr(arm.body, in_tail_position);
    uint32_t then_end = current_block_;
    IRInstr br_then;
    br_then.op = IROpcode::Branch;
    br_then.target_block = merge_block;
    emit(br_then);

    // Else: next arm chain
    switchToBlock(else_block);
    ValueId else_val = buildMatchChain(matchE, scrutinee, arm_idx + 1, in_tail_position);
    uint32_t else_end = current_block_;
    IRInstr br_else;
    br_else.op = IROpcode::Branch;
    br_else.target_block = merge_block;
    emit(br_else);

    // Merge
    switchToBlock(merge_block);
    ValueId merge_val = newValue();
    current_fn_->blocks[merge_block].params = {
        merge_val, then_val, else_val,
        static_cast<ValueId>(then_end),
        static_cast<ValueId>(else_end)
    };
    current_fn_->blocks[merge_block].is_merge = true;
    return merge_val;
}

void IRBuilder::buildStmt(Stmt* stmt) {
    switch (stmt->kind) {
        case Stmt::Kind::ValDecl: {
            auto* decl = static_cast<ValDeclStmt*>(stmt);
            ValueId val = buildExpr(decl->init);
            locals_[decl->name] = val;
            break;
        }
        case Stmt::Kind::VarDecl: {
            auto* decl = static_cast<VarDeclStmt*>(stmt);
            ValueId val = buildExpr(decl->init);
            locals_[decl->name] = val;
            break;
        }
        case Stmt::Kind::Assign: {
            auto* assign = static_cast<AssignStmt*>(stmt);
            ValueId val = buildExpr(assign->value);
            locals_[assign->name] = val; // SSA rename: update binding
            break;
        }
        case Stmt::Kind::ExprStmt:
            buildExpr(static_cast<ExprStmt*>(stmt)->expr);
            break;
        case Stmt::Kind::FieldAssign: {
            auto* fa_stmt = static_cast<FieldAssignStmt*>(stmt);
            ValueId val = buildExpr(fa_stmt->value);

            // Walk the FieldAccess chain to find base + total offset
            // e.g. s.pos.x → chain is [FieldAccess(FieldAccess(Ident "s", "pos"), "x")]
            // We need: base = locals_["s"], offset = pos.offset + x.offset within pos
            struct ChainEntry { std::string_view field_name; };
            std::vector<ChainEntry> chain;
            Expr* cur = fa_stmt->target;
            while (cur->kind == Expr::Kind::FieldAccess) {
                auto* fae = static_cast<FieldAccessExpr*>(cur);
                chain.push_back({fae->field_name});
                cur = fae->object;
            }
            // cur should be Ident (the base variable)
            assert(cur->kind == Expr::Kind::Ident);
            auto* base_ident = static_cast<IdentExpr*>(cur);
            ValueId base = locals_.at(base_ident->name);

            // Resolve struct name for the base
            std::string_view sname_sv = tc_->localStructName(base_ident->name);
            std::string sname(sname_sv);

            // Walk chain in reverse (outermost field first) to accumulate offset
            int32_t total_offset = 0;
            IRType store_type = IRType::Unknown;
            for (auto it = chain.rbegin(); it != chain.rend(); ++it) {
                auto* info = getStructInfo(sname);
                assert(info && "struct info not found for field assign");
                for (const auto& finfo : info->fields) {
                    if (finfo.name == it->field_name) {
                        total_offset += finfo.offset;
                        store_type = finfo.type;
                        if (!finfo.struct_name.empty()) {
                            sname = std::string(finfo.struct_name);
                        }
                        break;
                    }
                }
            }

            IRInstr store;
            store.op = IROpcode::FieldStore;
            store.operands = {base, val};
            store.imm_value = total_offset;
            store.loc = stmt->loc;
            store.type = store_type;
            emit(store);
            break;
        }
    }
}

} // namespace kern
