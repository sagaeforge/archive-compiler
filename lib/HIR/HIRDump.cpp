#include "kern/hir/HIRDump.h"
#include "kern/support/TypeSystem.h"

namespace kern {

static void indent(std::ostream& out, int level) {
    for (int i = 0; i < level; ++i) out << "  ";
}

static const char* binOpName(HIRBinOp op) {
    switch (op) {
        case HIRBinOp::Add:   return "+";
        case HIRBinOp::Sub:   return "-";
        case HIRBinOp::Mul:   return "*";
        case HIRBinOp::Div:   return "/";
        case HIRBinOp::Eq:    return "==";
        case HIRBinOp::NotEq: return "!=";
        case HIRBinOp::Lt:    return "<";
        case HIRBinOp::LtEq:  return "<=";
        case HIRBinOp::Gt:    return ">";
        case HIRBinOp::GtEq:  return ">=";
        case HIRBinOp::And:    return "&&";
        case HIRBinOp::Or:     return "||";
        case HIRBinOp::Mod:    return "%";
        case HIRBinOp::BitAnd: return "&";
        case HIRBinOp::BitOr:  return "|";
        case HIRBinOp::BitXor: return "^";
        case HIRBinOp::Shl:    return "<<";
        case HIRBinOp::Shr:    return ">>";
    }
    return "?";
}

static const char* unaryOpName(HIRUnaryOp op) {
    switch (op) {
        case HIRUnaryOp::Neg:       return "-";
        case HIRUnaryOp::Not:       return "!";
        case HIRUnaryOp::BitNot:    return "~";
        case HIRUnaryOp::Deref:     return "*";
        case HIRUnaryOp::AddrOf:    return "&";
        case HIRUnaryOp::AddrOfVar: return "&var ";
    }
    return "?";
}

static void dumpPattern(const HIRPattern* pat, const TypeTable& types, std::ostream& out) {
    if (!pat) { out << "<null>"; return; }
    switch (pat->kind) {
        case HIRPattern::Kind::IntLit:
            out << static_cast<const HIRIntLitPattern*>(pat)->value;
            break;
        case HIRPattern::Kind::BoolLit:
            out << (static_cast<const HIRBoolLitPattern*>(pat)->value ? "true" : "false");
            break;
        case HIRPattern::Kind::Wildcard:
            out << "_";
            break;
        case HIRPattern::Kind::Variable:
            out << static_cast<const HIRVariablePattern*>(pat)->name;
            break;
        case HIRPattern::Kind::Enum:
            out << "." << static_cast<const HIREnumPattern*>(pat)->variant_name;
            break;
        case HIRPattern::Kind::Union: {
            auto* u = static_cast<const HIRUnionPattern*>(pat);
            out << "::" << u->variant_name;
            if (u->inner) {
                out << "(";
                dumpPattern(u->inner, types, out);
                out << ")";
            }
            if (u->field_bindings && u->field_binding_count > 0) {
                out << "{ ";
                for (uint32_t i = 0; i < u->field_binding_count; ++i) {
                    if (i > 0) out << ", ";
                    out << u->field_bindings[i].field_name << ": "
                        << u->field_bindings[i].binding_name;
                }
                out << " }";
            }
            break;
        }
    }
}

void dumpHIRExpr(const HIRExpr* expr, const TypeTable& types, std::ostream& out, int ind) {
    if (!expr) { indent(out, ind); out << "<null>\n"; return; }

    indent(out, ind);
    out << "(";

    switch (expr->kind) {
        case HIRExpr::Kind::IntLit:
            out << "int_lit " << static_cast<const HIRIntLitExpr*>(expr)->value;
            break;

        case HIRExpr::Kind::FloatLit:
            out << "float_lit " << static_cast<const HIRFloatLitExpr*>(expr)->value;
            break;

        case HIRExpr::Kind::BoolLit:
            out << "bool_lit " << (static_cast<const HIRBoolLitExpr*>(expr)->value ? "true" : "false");
            break;

        case HIRExpr::Kind::StringLit: {
            auto* s = static_cast<const HIRStringLitExpr*>(expr);
            out << "string_lit \"";
            for (uint32_t i = 0; i < s->length; ++i) {
                char c = s->data[i];
                switch (c) {
                    case '\n': out << "\\n"; break;
                    case '\t': out << "\\t"; break;
                    case '\\': out << "\\\\"; break;
                    case '"':  out << "\\\""; break;
                    default:   out << c; break;
                }
            }
            out << "\"";
            break;
        }

        case HIRExpr::Kind::Ident:
            out << "ident " << static_cast<const HIRIdentExpr*>(expr)->name;
            break;

        case HIRExpr::Kind::BinOp: {
            auto* b = static_cast<const HIRBinOpExpr*>(expr);
            out << "binop " << binOpName(b->op) << "\n";
            dumpHIRExpr(b->lhs, types, out, ind + 1);
            dumpHIRExpr(b->rhs, types, out, ind + 1);
            indent(out, ind);
            out << ")";
            out << " : " << types.name(expr->type) << "\n";
            return;
        }

        case HIRExpr::Kind::UnaryOp: {
            auto* u = static_cast<const HIRUnaryOpExpr*>(expr);
            out << "unary " << unaryOpName(u->op) << "\n";
            dumpHIRExpr(u->operand, types, out, ind + 1);
            indent(out, ind);
            out << ")";
            out << " : " << types.name(expr->type) << "\n";
            return;
        }

        case HIRExpr::Kind::Call: {
            auto* c = static_cast<const HIRCallExpr*>(expr);
            out << "call " << c->callee;
            if (c->is_tail_call) out << " [tail]";
            out << "\n";
            for (uint32_t i = 0; i < c->arg_count; ++i)
                dumpHIRExpr(c->args[i], types, out, ind + 1);
            indent(out, ind);
            out << ")";
            out << " : " << types.name(expr->type) << "\n";
            return;
        }

        case HIRExpr::Kind::If: {
            auto* f = static_cast<const HIRIfExpr*>(expr);
            out << "if\n";
            dumpHIRExpr(f->condition, types, out, ind + 1);
            indent(out, ind + 1); out << "then:\n";
            dumpHIRExpr(f->then_branch, types, out, ind + 2);
            if (f->else_branch) {
                indent(out, ind + 1); out << "else:\n";
                dumpHIRExpr(f->else_branch, types, out, ind + 2);
            }
            indent(out, ind);
            out << ")";
            out << " : " << types.name(expr->type) << "\n";
            return;
        }

        case HIRExpr::Kind::Match: {
            auto* m = static_cast<const HIRMatchExpr*>(expr);
            out << "match\n";
            dumpHIRExpr(m->scrutinee, types, out, ind + 1);
            for (uint32_t i = 0; i < m->arm_count; ++i) {
                indent(out, ind + 1);
                out << "arm ";
                dumpPattern(m->arms[i].pattern, types, out);
                if (m->arms[i].guard) {
                    out << " if ";
                    // inline guard dump
                }
                out << " =>\n";
                dumpHIRExpr(m->arms[i].body, types, out, ind + 2);
            }
            indent(out, ind);
            out << ")";
            out << " : " << types.name(expr->type) << "\n";
            return;
        }

        case HIRExpr::Kind::Block: {
            auto* b = static_cast<const HIRBlockExpr*>(expr);
            out << "block\n";
            for (uint32_t i = 0; i < b->stmt_count; ++i)
                dumpHIRStmt(b->stmts[i], types, out, ind + 1);
            if (b->result) {
                indent(out, ind + 1); out << "result:\n";
                dumpHIRExpr(b->result, types, out, ind + 2);
            }
            indent(out, ind);
            out << ")";
            out << " : " << types.name(expr->type) << "\n";
            return;
        }

        case HIRExpr::Kind::Return: {
            auto* r = static_cast<const HIRReturnExpr*>(expr);
            out << "return";
            if (r->value) {
                out << "\n";
                dumpHIRExpr(r->value, types, out, ind + 1);
                indent(out, ind);
                out << ")";
            } else {
                out << ")";
            }
            out << " : " << types.name(expr->type) << "\n";
            return;
        }

        case HIRExpr::Kind::StructLit: {
            auto* s = static_cast<const HIRStructLitExpr*>(expr);
            out << "struct_lit " << s->struct_name << "\n";
            for (uint32_t i = 0; i < s->field_count; ++i) {
                indent(out, ind + 1);
                out << s->fields[i].name << " =\n";
                dumpHIRExpr(s->fields[i].value, types, out, ind + 2);
            }
            indent(out, ind);
            out << ")";
            out << " : " << types.name(expr->type) << "\n";
            return;
        }

        case HIRExpr::Kind::FieldAccess: {
            auto* f = static_cast<const HIRFieldAccessExpr*>(expr);
            out << "field_access ." << f->field_name << "\n";
            dumpHIRExpr(f->object, types, out, ind + 1);
            indent(out, ind);
            out << ")";
            out << " : " << types.name(expr->type) << "\n";
            return;
        }

        case HIRExpr::Kind::EnumAccess: {
            auto* e = static_cast<const HIREnumAccessExpr*>(expr);
            out << "enum_access " << e->enum_name << "." << e->variant_name;
            break;
        }

        case HIRExpr::Kind::UnionVariant: {
            auto* u = static_cast<const HIRUnionVariantExpr*>(expr);
            out << "union_variant " << u->union_name << "::" << u->variant_name;
            if (u->payload) {
                out << "\n";
                dumpHIRExpr(u->payload, types, out, ind + 1);
                indent(out, ind);
                out << ")";
                out << " : " << types.name(expr->type) << "\n";
                return;
            }
            break;
        }

        case HIRExpr::Kind::AddrOf: {
            auto* a = static_cast<const HIRAddrOfExpr*>(expr);
            out << (a->is_mutable ? "addr_of_var" : "addr_of") << "\n";
            dumpHIRExpr(a->operand, types, out, ind + 1);
            indent(out, ind);
            out << ")";
            out << " : " << types.name(expr->type) << "\n";
            return;
        }

        case HIRExpr::Kind::Deref: {
            auto* d = static_cast<const HIRDerefExpr*>(expr);
            out << "deref\n";
            dumpHIRExpr(d->operand, types, out, ind + 1);
            indent(out, ind);
            out << ")";
            out << " : " << types.name(expr->type) << "\n";
            return;
        }

        case HIRExpr::Kind::Cast: {
            auto* c = static_cast<const HIRCastExpr*>(expr);
            out << "cast\n";
            dumpHIRExpr(c->operand, types, out, ind + 1);
            indent(out, ind);
            out << "as " << types.name(c->target_type) << ")";
            out << " : " << types.name(expr->type) << "\n";
            return;
        }

        case HIRExpr::Kind::Loop: {
            auto* l = static_cast<const HIRLoopExpr*>(expr);
            out << "loop";
            if (l->binding_count > 0) {
                out << "(";
                for (uint32_t i = 0; i < l->binding_count; ++i) {
                    if (i > 0) out << ", ";
                    out << l->bindings[i].name << " = ";
                    // Inline init dump would be verbose; print on next line
                }
                out << ")\n";
                for (uint32_t i = 0; i < l->binding_count; ++i) {
                    indent(out, ind + 1);
                    out << l->bindings[i].name << " =\n";
                    dumpHIRExpr(l->bindings[i].init, types, out, ind + 2);
                }
            } else {
                out << "\n";
            }
            indent(out, ind + 1); out << "body:\n";
            dumpHIRExpr(l->body, types, out, ind + 2);
            indent(out, ind);
            out << ")";
            out << " : " << types.name(expr->type) << "\n";
            return;
        }

        case HIRExpr::Kind::Break: {
            auto* b = static_cast<const HIRBreakExpr*>(expr);
            out << "break";
            if (b->value) {
                out << "\n";
                dumpHIRExpr(b->value, types, out, ind + 1);
                indent(out, ind);
                out << ")";
            } else {
                out << ")";
            }
            out << " : " << types.name(expr->type) << "\n";
            return;
        }

        case HIRExpr::Kind::Continue: {
            auto* c = static_cast<const HIRContinueExpr*>(expr);
            out << "continue";
            if (c->arg_count > 0) {
                out << "(";
                for (uint32_t i = 0; i < c->arg_count; ++i) {
                    if (i > 0) out << ", ";
                }
                out << ")\n";
                for (uint32_t i = 0; i < c->arg_count; ++i) {
                    dumpHIRExpr(c->args[i], types, out, ind + 1);
                }
                indent(out, ind);
                out << ")";
            } else {
                out << ")";
            }
            out << " : " << types.name(expr->type) << "\n";
            return;
        }

        case HIRExpr::Kind::ArrayLit: {
            auto* a = static_cast<const HIRArrayLitExpr*>(expr);
            out << "array_lit\n";
            for (uint32_t i = 0; i < a->element_count; ++i)
                dumpHIRExpr(a->elements[i], types, out, ind + 1);
            indent(out, ind);
            out << ")";
            out << " : " << types.name(expr->type) << "\n";
            return;
        }

        case HIRExpr::Kind::IndexAccess: {
            auto* ia = static_cast<const HIRIndexAccessExpr*>(expr);
            out << "index_access\n";
            dumpHIRExpr(ia->array, types, out, ind + 1);
            dumpHIRExpr(ia->index, types, out, ind + 1);
            indent(out, ind);
            out << ")";
            out << " : " << types.name(expr->type) << "\n";
            return;
        }

        case HIRExpr::Kind::InlineAsm: {
            auto* ia = static_cast<const HIRInlineAsmExpr*>(expr);
            out << "inline_asm " << ia->line_count << " lines";
            break;
        }

        case HIRExpr::Kind::FnRef: {
            auto* f = static_cast<const HIRFnRefExpr*>(expr);
            out << "fn_ref " << f->fn_name;
            break;
        }

        case HIRExpr::Kind::CallIndirect: {
            auto* c = static_cast<const HIRCallIndirectExpr*>(expr);
            out << "call_indirect";
            if (c->is_tail_call) out << " [tail]";
            out << "\n";
            indent(out, ind + 1); out << "callee:\n";
            dumpHIRExpr(c->callee, types, out, ind + 2);
            for (uint32_t i = 0; i < c->arg_count; ++i)
                dumpHIRExpr(c->args[i], types, out, ind + 1);
            indent(out, ind);
            out << ")";
            out << " : " << types.name(expr->type) << "\n";
            return;
        }
    }

    // Simple nodes (leaf expressions) — type printed inline
    out << ") : " << types.name(expr->type) << "\n";
}

void dumpHIRStmt(const HIRStmt* stmt, const TypeTable& types, std::ostream& out, int ind) {
    if (!stmt) return;
    indent(out, ind);
    switch (stmt->kind) {
        case HIRStmt::Kind::ValDecl: {
            auto* v = static_cast<const HIRValDeclStmt*>(stmt);
            out << "(val " << v->name << " : " << types.name(v->type) << "\n";
            dumpHIRExpr(v->init, types, out, ind + 1);
            indent(out, ind); out << ")\n";
            break;
        }
        case HIRStmt::Kind::VarDecl: {
            auto* v = static_cast<const HIRVarDeclStmt*>(stmt);
            out << "(var " << v->name << " : " << types.name(v->type) << "\n";
            dumpHIRExpr(v->init, types, out, ind + 1);
            indent(out, ind); out << ")\n";
            break;
        }
        case HIRStmt::Kind::ExprStmt: {
            auto* e = static_cast<const HIRExprStmt*>(stmt);
            out << "(expr_stmt\n";
            dumpHIRExpr(e->expr, types, out, ind + 1);
            indent(out, ind); out << ")\n";
            break;
        }
        case HIRStmt::Kind::Assign: {
            auto* a = static_cast<const HIRAssignStmt*>(stmt);
            out << "(assign " << a->name << "\n";
            dumpHIRExpr(a->value, types, out, ind + 1);
            indent(out, ind); out << ")\n";
            break;
        }
        case HIRStmt::Kind::FieldAssign: {
            auto* f = static_cast<const HIRFieldAssignStmt*>(stmt);
            out << "(field_assign\n";
            indent(out, ind + 1); out << "target:\n";
            dumpHIRExpr(f->target, types, out, ind + 2);
            indent(out, ind + 1); out << "value:\n";
            dumpHIRExpr(f->value, types, out, ind + 2);
            indent(out, ind); out << ")\n";
            break;
        }
        case HIRStmt::Kind::DerefAssign: {
            auto* d = static_cast<const HIRDerefAssignStmt*>(stmt);
            out << "(deref_assign\n";
            indent(out, ind + 1); out << "target:\n";
            dumpHIRExpr(d->target, types, out, ind + 2);
            indent(out, ind + 1); out << "value:\n";
            dumpHIRExpr(d->value, types, out, ind + 2);
            indent(out, ind); out << ")\n";
            break;
        }
        case HIRStmt::Kind::IndexAssign: {
            auto* ia = static_cast<const HIRIndexAssignStmt*>(stmt);
            out << "(index_assign\n";
            indent(out, ind + 1); out << "array:\n";
            dumpHIRExpr(ia->array, types, out, ind + 2);
            indent(out, ind + 1); out << "index:\n";
            dumpHIRExpr(ia->index, types, out, ind + 2);
            indent(out, ind + 1); out << "value:\n";
            dumpHIRExpr(ia->value, types, out, ind + 2);
            indent(out, ind); out << ")\n";
            break;
        }
    }
}

static const char* purityName(uint8_t p) {
    switch (p) {
        case 0: return "pure";
        case 1: return "impure(mut)";
        case 2: return "impure(io)";
        case 3: return "impure(mem)";
        default: return "unknown";
    }
}

void dumpHIR(const HIRModule* mod, const TypeTable& types, std::ostream& out) {
    if (!mod) return;

    // Struct declarations
    for (uint32_t i = 0; i < mod->struct_count; ++i) {
        auto* s = mod->structs[i];
        out << "(struct_decl " << s->name << " type_id=" << s->type_id << ")\n";
    }

    // Enum declarations
    for (uint32_t i = 0; i < mod->enum_count; ++i) {
        auto* e = mod->enums[i];
        out << "(enum_decl " << e->name << " type_id=" << e->type_id << ")\n";
    }

    // Union declarations
    for (uint32_t i = 0; i < mod->union_count; ++i) {
        auto* u = mod->unions[i];
        out << "(union_decl " << u->name << " type_id=" << u->type_id << ")\n";
    }

    // Function declarations
    for (uint32_t i = 0; i < mod->fn_count; ++i) {
        auto* fn = mod->functions[i];
        out << "(fn " << fn->name << " [" << purityName(fn->purity) << "]";
        if (fn->is_recursive) out << " [recursive]";
        if (fn->is_tail_recursive) out << " [tail-recursive]";
        if (fn->is_intrinsic) out << " [intrinsic]";
        out << "\n";

        // Parameters
        out << "  params: (";
        for (uint32_t j = 0; j < fn->param_count; ++j) {
            if (j > 0) out << ", ";
            out << fn->params[j].name << " : " << types.name(fn->params[j].type);
        }
        out << ") -> " << types.name(fn->return_type) << "\n";

        // Body
        if (fn->body) {
            out << "  body:\n";
            dumpHIRExpr(fn->body, types, out, 2);
        }
        out << ")\n\n";
    }
}

} // namespace kern
