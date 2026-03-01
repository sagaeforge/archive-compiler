#include "kern/ir/IRBuilder.h"
#include <cassert>
#include <string>

namespace kern {

const char* irOpcodeName(IROpcode op) {
    switch (op) {
        case IROpcode::ConstInt:   return "const_int";
        case IROpcode::Add:        return "add";
        case IROpcode::Sub:        return "sub";
        case IROpcode::Mul:        return "mul";
        case IROpcode::Div:        return "div";
        case IROpcode::ICmpEq:     return "icmp_eq";
        case IROpcode::ICmpNe:     return "icmp_ne";
        case IROpcode::ICmpLt:     return "icmp_lt";
        case IROpcode::ICmpLe:     return "icmp_le";
        case IROpcode::ICmpGt:     return "icmp_gt";
        case IROpcode::ICmpGe:     return "icmp_ge";
        case IROpcode::Neg:        return "neg";
        case IROpcode::Not:        return "not";
        case IROpcode::Branch:     return "br";
        case IROpcode::CondBranch: return "condbr";
        case IROpcode::Ret:        return "ret";
        case IROpcode::Call:       return "call";
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
        out << ") -> " << fn.return_type_name << " {\n";

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
                out << irOpcodeName(instr.op);

                switch (instr.op) {
                    case IROpcode::ConstInt:
                        out << " " << instr.imm_value;
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

IRModule IRBuilder::build(Module* mod, const TypeChecker& tc) {
    tc_ = &tc;
    module_.functions.clear();
    for (uint32_t i = 0; i < mod->fn_count; ++i) {
        buildFunction(mod->functions[i]);
    }
    return std::move(module_);
}

void IRBuilder::buildFunction(FnDecl* fn) {
    module_.functions.push_back({});
    current_fn_ = &module_.functions.back();
    current_fn_->name = std::string(fn->name);
    current_fn_->return_type_name = std::string(fn->return_type.name);
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
        auto name = fn->params[i].type.name;
        IRType pt = IRType::Unknown;
        if (name == "i8")        pt = IRType::I8;
        else if (name == "i16")  pt = IRType::I16;
        else if (name == "i32")  pt = IRType::I32;
        else if (name == "i64")  pt = IRType::I64;
        else if (name == "u8")   pt = IRType::U8;
        else if (name == "u16")  pt = IRType::U16;
        else if (name == "u32")  pt = IRType::U32;
        else if (name == "u64")  pt = IRType::U64;
        else if (name == "bool") pt = IRType::Bool;
        current_fn_->param_types.push_back(pt);
        locals_[fn->params[i].name] = pv;
    }

    ValueId result = buildExpr(fn->body);

    auto& instrs = current_fn_->blocks[current_block_].instrs;
    if (instrs.empty() || instrs.back().op != IROpcode::Ret) {
        IRInstr ret;
        ret.op = IROpcode::Ret;
        ret.operands = {result};
        emit(ret);
    }
}

ValueId IRBuilder::buildExpr(Expr* expr) {
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
            ValueId lhs = buildExpr(bin->lhs);
            ValueId rhs = buildExpr(bin->rhs);

            IROpcode op;
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
                case BinOpKind::And:   op = IROpcode::ICmpEq; break;
                case BinOpKind::Or:    op = IROpcode::ICmpEq; break;
            }

            if (bin->op == BinOpKind::And) {
                op = IROpcode::Mul;
            } else if (bin->op == BinOpKind::Or) {
                IRInstr add;
                add.op = IROpcode::Add;
                add.result = newValue();
                add.operands = {lhs, rhs};
                add.loc = expr->loc;
                add.type = IRType::Bool;
                ValueId sum = emit(add);

                IRInstr zero;
                zero.op = IROpcode::ConstInt;
                zero.result = newValue();
                zero.imm_value = 0;
                zero.type = IRType::Bool;
                ValueId zeroVal = emit(zero);

                IRInstr cmp;
                cmp.op = IROpcode::ICmpNe;
                cmp.result = newValue();
                cmp.operands = {sum, zeroVal};
                cmp.loc = expr->loc;
                cmp.type = IRType::Bool;
                return emit(cmp);
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
                IRInstr zero;
                zero.op = IROpcode::ConstInt;
                zero.result = newValue();
                zero.imm_value = 0;
                zero.loc = expr->loc;
                zero.type = expr_type;
                ValueId zeroVal = emit(zero);

                IRInstr sub;
                sub.op = IROpcode::Sub;
                sub.result = newValue();
                sub.operands = {zeroVal, operand};
                sub.loc = expr->loc;
                sub.type = expr_type;
                return emit(sub);
            } else if (unary->op == UnaryOpKind_t::Not) {
                IRInstr one;
                one.op = IROpcode::ConstInt;
                one.result = newValue();
                one.imm_value = 1;
                one.loc = expr->loc;
                one.type = IRType::Bool;
                ValueId oneVal = emit(one);

                IRInstr sub;
                sub.op = IROpcode::Sub;
                sub.result = newValue();
                sub.operands = {oneVal, operand};
                sub.loc = expr->loc;
                sub.type = IRType::Bool;
                return emit(sub);
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
            ValueId then_val = buildExpr(ifE->then_branch);
            uint32_t then_end_block = current_block_;

            IRInstr br_then;
            br_then.op = IROpcode::Branch;
            br_then.target_block = merge_block;
            emit(br_then);

            switchToBlock(else_block);
            ValueId else_val;
            if (ifE->else_branch) {
                else_val = buildExpr(ifE->else_branch);
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

            return merge_val;
        }

        case Expr::Kind::Block: {
            auto* block = static_cast<BlockExpr*>(expr);
            for (uint32_t i = 0; i < block->stmt_count; ++i) {
                buildStmt(block->stmts[i]);
            }
            if (block->result) {
                return buildExpr(block->result);
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
                val = buildExpr(ret->value);
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
    }

    IRInstr dummy;
    dummy.op = IROpcode::ConstInt;
    dummy.result = newValue();
    dummy.imm_value = 0;
    return emit(dummy);
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
        case Stmt::Kind::ExprStmt:
            buildExpr(static_cast<ExprStmt*>(stmt)->expr);
            break;
    }
}

} // namespace kern
