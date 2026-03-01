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
        out << "fn " << fn.name << "(";
        for (size_t i = 0; i < fn.param_values.size(); ++i) {
            if (i > 0) out << ", ";
            out << "%" << fn.param_names[i] << " = %" << fn.param_values[i];
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
                    out << "%" << instr.result << " = ";
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

IRModule IRBuilder::build(Module* mod) {
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

    uint32_t entry = newBlock("entry");
    switchToBlock(entry);

    // Register parameters
    for (uint32_t i = 0; i < fn->param_count; ++i) {
        ValueId pv = newValue();
        current_fn_->param_values.push_back(pv);
        current_fn_->param_names.push_back(std::string(fn->params[i].name));
        locals_[fn->params[i].name] = pv;
    }

    ValueId result = buildExpr(fn->body);

    // If the last instruction is not already a ret, emit one
    auto& instrs = current_fn_->blocks[current_block_].instrs;
    if (instrs.empty() || instrs.back().op != IROpcode::Ret) {
        IRInstr ret;
        ret.op = IROpcode::Ret;
        ret.operands = {result};
        emit(ret);
    }
}

ValueId IRBuilder::buildExpr(Expr* expr) {
    switch (expr->kind) {
        case Expr::Kind::IntLit: {
            auto* lit = static_cast<IntLitExpr*>(expr);
            IRInstr instr;
            instr.op = IROpcode::ConstInt;
            instr.result = newValue();
            instr.imm_value = lit->value;
            instr.loc = expr->loc;
            return emit(instr);
        }

        case Expr::Kind::BoolLit: {
            auto* lit = static_cast<BoolLitExpr*>(expr);
            IRInstr instr;
            instr.op = IROpcode::ConstInt;
            instr.result = newValue();
            instr.imm_value = lit->value ? 1 : 0;
            instr.loc = expr->loc;
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
                // For M1, and/or are just treated as bitwise since sema ensures bool
                case BinOpKind::And:   op = IROpcode::ICmpEq; break;
                case BinOpKind::Or:    op = IROpcode::ICmpEq; break;
            }

            // Handle And/Or specially with multiplication/addition for boolean logic
            if (bin->op == BinOpKind::And) {
                // a and b = a * b (both 0 or 1)
                op = IROpcode::Mul;
            } else if (bin->op == BinOpKind::Or) {
                // a or b => (a + b) != 0, but simpler: add + icmp_ne 0
                IRInstr add;
                add.op = IROpcode::Add;
                add.result = newValue();
                add.operands = {lhs, rhs};
                add.loc = expr->loc;
                ValueId sum = emit(add);

                IRInstr zero;
                zero.op = IROpcode::ConstInt;
                zero.result = newValue();
                zero.imm_value = 0;
                ValueId zeroVal = emit(zero);

                IRInstr cmp;
                cmp.op = IROpcode::ICmpNe;
                cmp.result = newValue();
                cmp.operands = {sum, zeroVal};
                cmp.loc = expr->loc;
                return emit(cmp);
            }

            IRInstr instr;
            instr.op = op;
            instr.result = newValue();
            instr.operands = {lhs, rhs};
            instr.loc = expr->loc;
            return emit(instr);
        }

        case Expr::Kind::UnaryOp: {
            auto* unary = static_cast<UnaryOpExpr*>(expr);
            ValueId operand = buildExpr(unary->operand);

            if (unary->op == UnaryOpKind_t::Neg) {
                // 0 - operand
                IRInstr zero;
                zero.op = IROpcode::ConstInt;
                zero.result = newValue();
                zero.imm_value = 0;
                zero.loc = expr->loc;
                ValueId zeroVal = emit(zero);

                IRInstr sub;
                sub.op = IROpcode::Sub;
                sub.result = newValue();
                sub.operands = {zeroVal, operand};
                sub.loc = expr->loc;
                return emit(sub);
            } else if (unary->op == UnaryOpKind_t::Not) {
                // 1 - operand (for bool)
                IRInstr one;
                one.op = IROpcode::ConstInt;
                one.result = newValue();
                one.imm_value = 1;
                one.loc = expr->loc;
                ValueId oneVal = emit(one);

                IRInstr sub;
                sub.op = IROpcode::Sub;
                sub.result = newValue();
                sub.operands = {oneVal, operand};
                sub.loc = expr->loc;
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
            return emit(instr);
        }

        case Expr::Kind::If: {
            auto* ifE = static_cast<IfExpr*>(expr);
            ValueId cond = buildExpr(ifE->condition);

            uint32_t then_block = newBlock("then_" + std::to_string(label_counter_));
            uint32_t else_block = newBlock("else_" + std::to_string(label_counter_));
            uint32_t merge_block = newBlock("merge_" + std::to_string(label_counter_));
            label_counter_++;

            // Conditional branch
            IRInstr br;
            br.op = IROpcode::CondBranch;
            br.operands = {cond};
            br.target_block = then_block;
            br.false_block = else_block;
            emit(br);

            // Then block
            switchToBlock(then_block);
            ValueId then_val = buildExpr(ifE->then_branch);
            // Remember which block we ended up in (might have changed during buildExpr)
            uint32_t then_end_block = current_block_;

            IRInstr br_then;
            br_then.op = IROpcode::Branch;
            br_then.target_block = merge_block;
            emit(br_then);

            // Else block
            switchToBlock(else_block);
            ValueId else_val;
            if (ifE->else_branch) {
                else_val = buildExpr(ifE->else_branch);
            } else {
                IRInstr unit;
                unit.op = IROpcode::ConstInt;
                unit.result = newValue();
                unit.imm_value = 0;
                else_val = emit(unit);
            }
            uint32_t else_end_block = current_block_;

            IRInstr br_else;
            br_else.op = IROpcode::Branch;
            br_else.target_block = merge_block;
            emit(br_else);

            // Merge block — use a simple approach: store then/else values
            // For M1 simplicity, we track the values via a pseudo-phi
            // We'll handle this in codegen by tracking which value comes from which block
            switchToBlock(merge_block);

            // Store merge info as block params
            current_fn_->blocks[merge_block].params = {then_val, else_val};
            // Also store source blocks for codegen
            // We encode: params[0] = then_val, params[1] = else_val
            // And store source block indices in a hack via extra params
            current_fn_->blocks[merge_block].params.push_back(then_end_block);
            current_fn_->blocks[merge_block].params.push_back(else_end_block);

            // The merge value is a new SSA value
            ValueId merge_val = newValue();
            // We don't emit an instruction for the merge — codegen handles it
            // Store merge_val as first param (overwrite)
            // Actually let's use a cleaner approach: just return one of the values
            // and let codegen figure out the phi
            // For simplicity in M1, let's just use a pseudo instruction

            // Clean approach: use the merge block's params to communicate phi info
            // params = [merge_result_value, then_val, else_val, then_block, else_block]
            current_fn_->blocks[merge_block].params.clear();
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
            // Unit return
            IRInstr unit;
            unit.op = IROpcode::ConstInt;
            unit.result = newValue();
            unit.imm_value = 0;
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
                val = emit(unit);
            }
            IRInstr retInstr;
            retInstr.op = IROpcode::Ret;
            retInstr.operands = {val};
            emit(retInstr);
            return val;
        }
    }

    // Unreachable
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
