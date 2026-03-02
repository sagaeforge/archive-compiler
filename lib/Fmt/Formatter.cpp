#include "kern/fmt/Formatter.h"
#include <cstring>

namespace kern {

// ============================================================================
// Module formatting
// ============================================================================

void Formatter::formatModule(const Module* mod) {
    bool first = true;

    auto sep = [&]() { if (!first) out_ << "\n"; first = false; };

    for (uint32_t i = 0; i < mod->import_count; ++i) {
        sep(); formatImportDecl(mod->imports[i]);
    }
    for (uint32_t i = 0; i < mod->type_alias_count; ++i) {
        sep(); formatTypeAliasDecl(mod->type_aliases[i]);
    }
    for (uint32_t i = 0; i < mod->newtype_count; ++i) {
        sep(); formatNewtypeDecl(mod->newtypes[i]);
    }
    for (uint32_t i = 0; i < mod->struct_count; ++i) {
        sep(); formatStructDecl(mod->structs[i]);
    }
    for (uint32_t i = 0; i < mod->enum_count; ++i) {
        sep(); formatEnumDecl(mod->enums[i]);
    }
    for (uint32_t i = 0; i < mod->union_count; ++i) {
        sep(); formatUnionDecl(mod->unions[i]);
    }
    for (uint32_t i = 0; i < mod->trait_count; ++i) {
        sep(); formatTraitDecl(mod->traits[i]);
    }
    for (uint32_t i = 0; i < mod->impl_count; ++i) {
        sep(); formatImplDecl(mod->impls[i]);
    }
    for (uint32_t i = 0; i < mod->global_count; ++i) {
        sep(); formatGlobalDecl(mod->globals[i]);
    }
    for (uint32_t i = 0; i < mod->static_assert_count; ++i) {
        sep(); formatStaticAssertDecl(mod->static_asserts[i]);
    }
    for (uint32_t i = 0; i < mod->fn_count; ++i) {
        sep(); formatFnDecl(mod->functions[i]);
    }

    out_ << "\n";
}

// ============================================================================
// Declaration formatting
// ============================================================================

void Formatter::formatStructDecl(const StructDecl* s) {
    out_ << "struct " << s->name << " {";
    if (s->field_count == 0) {
        out_ << "}\n";
        return;
    }
    out_ << "\n";
    indent();
    for (uint32_t i = 0; i < s->field_count; ++i) {
        writeIndent();
        if (!s->fields[i].is_pub) out_ << "priv ";
        if (s->fields[i].is_mutable) out_ << "var ";
        out_ << s->fields[i].name << ": ";
        formatTypeRef(s->fields[i].type);
        out_ << "\n";
    }
    dedent();
    out_ << "}\n";
}

void Formatter::formatEnumDecl(const EnumDecl* e) {
    out_ << "enum " << e->name << " {";
    if (e->variant_count == 0) {
        out_ << "}\n";
        return;
    }
    out_ << " ";
    for (uint32_t i = 0; i < e->variant_count; ++i) {
        if (i > 0) out_ << ", ";
        out_ << e->variants[i].name;
    }
    out_ << " }\n";
}

void Formatter::formatUnionDecl(const UnionDecl* u) {
    out_ << "union " << u->name << " {";
    if (u->variant_count == 0) {
        out_ << "}\n";
        return;
    }
    out_ << "\n";
    indent();
    for (uint32_t i = 0; i < u->variant_count; ++i) {
        writeIndent();
        out_ << u->variants[i].name;
        if (u->variants[i].payload_type) {
            out_ << "(";
            formatTypeRef(*u->variants[i].payload_type);
            out_ << ")";
        }
        out_ << "\n";
    }
    dedent();
    out_ << "}\n";
}

void Formatter::formatFnDecl(const FnDecl* fn) {
    out_ << "fn " << fn->name << "(";
    for (uint32_t i = 0; i < fn->param_count; ++i) {
        if (i > 0) out_ << ", ";
        out_ << fn->params[i].name << ": ";
        formatTypeRef(fn->params[i].type);
    }
    out_ << ")";
    if (!fn->return_type.name.empty() || fn->return_type.kind != TypeRef::Kind::Named) {
        out_ << " -> ";
        formatTypeRef(fn->return_type);
    }

    if (fn->is_intrinsic) {
        out_ << " = intrinsic\n";
        return;
    }

    if (!fn->body) {
        out_ << "\n";
        return;
    }

    out_ << " ";
    if (fn->body->kind == Expr::Kind::Block) {
        formatBlock(static_cast<const BlockExpr*>(fn->body), true);
    } else {
        out_ << "{\n";
        indent();
        writeIndent();
        formatExpr(fn->body);
        out_ << "\n";
        dedent();
        out_ << "}\n";
    }
}

void Formatter::formatImportDecl(const ImportDecl* imp) {
    out_ << "import " << imp->module_path;
    if (imp->name_count > 0) {
        out_ << " { ";
        for (uint32_t i = 0; i < imp->name_count; ++i) {
            if (i > 0) out_ << ", ";
            out_ << imp->names[i];
        }
        out_ << " }";
    }
    out_ << "\n";
}

void Formatter::formatTypeAliasDecl(const TypeAliasDecl* ta) {
    out_ << "type " << ta->name << " = ";
    formatTypeRef(ta->target);
    out_ << "\n";
}

void Formatter::formatNewtypeDecl(const NewtypeDecl* nt) {
    out_ << "newtype " << nt->name << "(";
    formatTypeRef(nt->inner);
    out_ << ")\n";
}

void Formatter::formatStaticAssertDecl(const StaticAssertDecl* sa) {
    out_ << "static_assert(";
    if (sa->condition) formatExpr(sa->condition);
    if (!sa->message.empty()) {
        out_ << ", \"" << sa->message << "\"";
    }
    out_ << ")\n";
}

void Formatter::formatTraitDecl(const TraitDecl* t) {
    out_ << "trait " << t->name << " {\n";
    indent();
    for (uint32_t i = 0; i < t->method_count; ++i) {
        auto& m = t->methods[i];
        writeIndent();
        out_ << "fn " << m.name << "(";
        for (uint32_t j = 0; j < m.param_count; ++j) {
            if (j > 0) out_ << ", ";
            out_ << m.params[j].name << ": ";
            formatTypeRef(m.params[j].type);
        }
        out_ << ") -> ";
        formatTypeRef(m.return_type);
        if (m.effect_count > 0) {
            out_ << " with ";
            for (uint32_t j = 0; j < m.effect_count; ++j) {
                if (j > 0) out_ << ", ";
                out_ << m.effect_names[j];
            }
        }
        out_ << "\n";
    }
    dedent();
    out_ << "}\n";
}

void Formatter::formatImplDecl(const ImplDecl* imp) {
    if (imp->trait_name.empty()) {
        out_ << "impl ";
    } else {
        out_ << "impl " << imp->trait_name << " for ";
    }
    formatTypeRef(imp->target_type);
    out_ << " {\n";
    indent();
    for (uint32_t i = 0; i < imp->method_count; ++i) {
        writeIndent();
        formatFnDecl(imp->methods[i]);
    }
    dedent();
    out_ << "}\n";
}

void Formatter::formatGlobalDecl(const GlobalDecl* g) {
    out_ << "static " << (g->is_mutable ? "var " : "val ");
    out_ << g->name << ": ";
    formatTypeRef(g->type);
    if (g->init) {
        out_ << " = ";
        formatExpr(g->init);
    }
    out_ << "\n";
}

// ============================================================================
// Type formatting
// ============================================================================

void Formatter::formatTypeRef(const TypeRef& t) {
    switch (t.kind) {
        case TypeRef::Kind::Named:
            out_ << t.name;
            if (t.type_arg_count > 0) {
                out_ << "<";
                for (uint32_t i = 0; i < t.type_arg_count; ++i) {
                    if (i > 0) out_ << ", ";
                    formatTypeRef(t.type_args[i]);
                }
                out_ << ">";
            }
            break;
        case TypeRef::Kind::Ptr:
            out_ << "Ptr<";
            if (t.is_ptr_var) out_ << "var ";
            if (t.pointee) formatTypeRef(*t.pointee);
            out_ << ">";
            break;
        case TypeRef::Kind::Fn:
            out_ << "fn(";
            for (uint32_t i = 0; i < t.fn_param_count; ++i) {
                if (i > 0) out_ << ", ";
                formatTypeRef(t.fn_params[i]);
            }
            out_ << ")";
            if (t.fn_return) {
                out_ << " -> ";
                formatTypeRef(*t.fn_return);
            }
            break;
        case TypeRef::Kind::Never:
            out_ << "Never";
            break;
        case TypeRef::Kind::Array:
            out_ << "[";
            if (t.array_element) formatTypeRef(*t.array_element);
            if (!t.array_size_name.empty())
                out_ << "; " << t.array_size_name << "]";
            else
                out_ << "; " << t.array_size << "]";
            break;
        case TypeRef::Kind::ConstVal:
            out_ << t.const_value;
            break;
        case TypeRef::Kind::Dyn:
            out_ << "dyn " << t.name;
            break;
        case TypeRef::Kind::Tuple:
            out_ << "(";
            for (uint32_t i = 0; i < t.tuple_count; ++i) {
                if (i > 0) out_ << ", ";
                formatTypeRef(t.tuple_elements[i]);
            }
            out_ << ")";
            break;
    }
}

// ============================================================================
// Expression formatting
// ============================================================================

void Formatter::formatExpr(const Expr* expr) {
    switch (expr->kind) {
        case Expr::Kind::IntLit:
            out_ << static_cast<const IntLitExpr*>(expr)->value;
            break;

        case Expr::Kind::FloatLit: {
            auto* f = static_cast<const FloatLitExpr*>(expr);
            out_ << f->value;
            if (f->is_f32) out_ << "f";
            break;
        }

        case Expr::Kind::BoolLit:
            out_ << (static_cast<const BoolLitExpr*>(expr)->value ? "true" : "false");
            break;

        case Expr::Kind::NullLit:
            out_ << "null";
            break;

        case Expr::Kind::StringLit: {
            auto* s = static_cast<const StringLitExpr*>(expr);
            out_ << '"';
            for (uint32_t i = 0; i < s->length; ++i) {
                char c = s->data[i];
                switch (c) {
                    case '\n': out_ << "\\n"; break;
                    case '\t': out_ << "\\t"; break;
                    case '\\': out_ << "\\\\"; break;
                    case '"': out_ << "\\\""; break;
                    default: out_ << c;
                }
            }
            out_ << '"';
            break;
        }

        case Expr::Kind::Ident:
            out_ << static_cast<const IdentExpr*>(expr)->name;
            break;

        case Expr::Kind::BinOp: {
            auto* b = static_cast<const BinOpExpr*>(expr);
            formatExpr(b->lhs);
            out_ << " " << binOpStr(b->op) << " ";
            formatExpr(b->rhs);
            break;
        }

        case Expr::Kind::UnaryOp: {
            auto* u = static_cast<const UnaryOpExpr*>(expr);
            switch (u->op) {
                case UnaryOpKind::Neg: out_ << "-"; break;
                case UnaryOpKind::Not: out_ << "not "; break;
                case UnaryOpKind::BitNot: out_ << "~"; break;
                case UnaryOpKind::Deref: out_ << "(*"; formatExpr(u->operand); out_ << ")"; return;
                case UnaryOpKind::AddrOf: out_ << "&"; break;
                case UnaryOpKind::AddrOfVar: out_ << "&var "; break;
            }
            formatExpr(u->operand);
            break;
        }

        case Expr::Kind::Call: {
            auto* c = static_cast<const CallExpr*>(expr);
            out_ << c->callee << "(";
            for (uint32_t i = 0; i < c->arg_count; ++i) {
                if (i > 0) out_ << ", ";
                formatExpr(c->args[i]);
            }
            out_ << ")";
            break;
        }

        case Expr::Kind::If: {
            auto* ife = static_cast<const IfExpr*>(expr);
            out_ << "if ";
            formatExpr(ife->condition);
            out_ << " ";
            if (ife->then_branch->kind == Expr::Kind::Block) {
                formatBlock(static_cast<const BlockExpr*>(ife->then_branch));
            } else {
                out_ << "{ ";
                formatExpr(ife->then_branch);
                out_ << " }";
            }
            if (ife->else_branch) {
                out_ << " else ";
                if (ife->else_branch->kind == Expr::Kind::If) {
                    formatExpr(ife->else_branch);
                } else if (ife->else_branch->kind == Expr::Kind::Block) {
                    formatBlock(static_cast<const BlockExpr*>(ife->else_branch));
                } else {
                    out_ << "{ ";
                    formatExpr(ife->else_branch);
                    out_ << " }";
                }
            }
            break;
        }

        case Expr::Kind::Block:
            formatBlock(static_cast<const BlockExpr*>(expr));
            break;

        case Expr::Kind::Return: {
            auto* r = static_cast<const ReturnExpr*>(expr);
            out_ << "return";
            if (r->value) {
                out_ << " ";
                formatExpr(r->value);
            }
            break;
        }

        case Expr::Kind::Match: {
            auto* m = static_cast<const MatchExpr*>(expr);
            out_ << "match ";
            formatExpr(m->scrutinee);
            out_ << " {\n";
            indent();
            for (uint32_t i = 0; i < m->arm_count; ++i) {
                formatMatchArm(m->arms[i]);
            }
            dedent();
            writeIndent();
            out_ << "}";
            break;
        }

        case Expr::Kind::StructLit: {
            auto* s = static_cast<const StructLitExpr*>(expr);
            out_ << s->struct_name << " { ";
            for (uint32_t i = 0; i < s->field_count; ++i) {
                if (i > 0) out_ << ", ";
                out_ << s->fields[i].name << ": ";
                formatExpr(s->fields[i].value);
            }
            out_ << " }";
            break;
        }

        case Expr::Kind::FieldAccess: {
            auto* f = static_cast<const FieldAccessExpr*>(expr);
            formatExpr(f->object);
            out_ << "." << f->field_name;
            break;
        }

        case Expr::Kind::EnumAccess: {
            auto* e = static_cast<const EnumAccessExpr*>(expr);
            out_ << e->enum_name << "." << e->variant_name;
            break;
        }

        case Expr::Kind::UnionVariant: {
            auto* u = static_cast<const UnionVariantExpr*>(expr);
            out_ << u->union_name << "::" << u->variant_name;
            if (u->payload) {
                out_ << "(";
                formatExpr(u->payload);
                out_ << ")";
            }
            break;
        }

        case Expr::Kind::Cast: {
            auto* c = static_cast<const CastExpr*>(expr);
            formatExpr(c->operand);
            out_ << " as ";
            formatTypeRef(c->target);
            break;
        }

        case Expr::Kind::Loop: {
            auto* lp = static_cast<const LoopExpr*>(expr);
            out_ << "loop";
            if (lp->binding_count > 0) {
                out_ << "(";
                for (uint32_t i = 0; i < lp->binding_count; ++i) {
                    if (i > 0) out_ << ", ";
                    out_ << lp->bindings[i].name << " = ";
                    formatExpr(lp->bindings[i].init);
                }
                out_ << ")";
            }
            out_ << " {\n";
            indent();
            for (uint32_t i = 0; i < lp->stmt_count; ++i) {
                formatStmt(lp->stmts[i]);
            }
            if (lp->result) {
                writeIndent();
                formatExpr(lp->result);
                out_ << "\n";
            }
            dedent();
            writeIndent();
            out_ << "}";
            break;
        }

        case Expr::Kind::WhileLoop: {
            auto* wl = static_cast<const WhileLoopExpr*>(expr);
            out_ << "while ";
            formatExpr(wl->condition);
            out_ << " {\n";
            indent();
            for (uint32_t i = 0; i < wl->stmt_count; ++i) {
                formatStmt(wl->stmts[i]);
            }
            dedent();
            writeIndent();
            out_ << "}";
            break;
        }

        case Expr::Kind::ForRange: {
            auto* fr = static_cast<const ForRangeExpr*>(expr);
            out_ << "for " << fr->var_name << " in ";
            formatExpr(fr->start);
            out_ << "..";
            formatExpr(fr->end);
            out_ << " {\n";
            indent();
            for (uint32_t i = 0; i < fr->stmt_count; ++i) {
                formatStmt(fr->stmts[i]);
            }
            dedent();
            writeIndent();
            out_ << "}";
            break;
        }
        case Expr::Kind::ForEach: {
            auto* fe = static_cast<const ForEachExpr*>(expr);
            out_ << "for " << fe->var_name << " in ";
            formatExpr(fe->collection);
            out_ << " {\n";
            indent();
            for (uint32_t i = 0; i < fe->stmt_count; ++i) {
                formatStmt(fe->stmts[i]);
            }
            dedent();
            writeIndent();
            out_ << "}";
            break;
        }

        case Expr::Kind::InlineAsm: {
            auto* ia = static_cast<const InlineAsmExpr*>(expr);
            out_ << "asm {\n";
            indent();
            for (uint32_t i = 0; i < ia->line_count; ++i) {
                writeIndent();
                out_ << "\"";
                for (uint32_t j = 0; j < ia->lines[i]->length; ++j) {
                    char c = ia->lines[i]->data[j];
                    if (c == '"') out_ << "\\\"";
                    else if (c == '\\') out_ << "\\\\";
                    else out_ << c;
                }
                out_ << "\"\n";
            }
            dedent();
            writeIndent();
            out_ << "}";
            break;
        }

        case Expr::Kind::ArrayLit: {
            auto* al = static_cast<const ArrayLitExpr*>(expr);
            out_ << "[";
            for (uint32_t i = 0; i < al->count; ++i) {
                if (i > 0) out_ << ", ";
                formatExpr(al->elements[i]);
            }
            out_ << "]";
            break;
        }

        case Expr::Kind::IndexAccess: {
            auto* ia = static_cast<const IndexAccessExpr*>(expr);
            formatExpr(ia->array);
            out_ << "[";
            formatExpr(ia->index);
            out_ << "]";
            break;
        }

        case Expr::Kind::SliceExpr: {
            auto* se = static_cast<const SliceExprNode*>(expr);
            formatExpr(se->array);
            out_ << "[";
            if (se->start) formatExpr(se->start);
            out_ << "..";
            if (se->end) formatExpr(se->end);
            out_ << "]";
            break;
        }

        case Expr::Kind::Sizeof: {
            auto* sz = static_cast<const SizeofExpr*>(expr);
            out_ << "sizeof(";
            formatTypeRef(sz->target);
            out_ << ")";
            break;
        }

        case Expr::Kind::Alignof: {
            auto* al = static_cast<const AlignofExpr*>(expr);
            out_ << "alignof(";
            formatTypeRef(al->target);
            out_ << ")";
            break;
        }

        case Expr::Kind::Offsetof: {
            auto* oe = static_cast<const OffsetofExpr*>(expr);
            out_ << "offsetof(";
            formatTypeRef(oe->target);
            out_ << ", " << oe->field_name << ")";
            break;
        }

        case Expr::Kind::Lambda: {
            auto* lam = static_cast<const LambdaExpr*>(expr);
            out_ << "{ ";
            for (uint32_t i = 0; i < lam->param_count; ++i) {
                if (i > 0) out_ << ", ";
                out_ << lam->params[i].name << ": ";
                formatTypeRef(lam->params[i].type);
            }
            if (lam->param_count > 0) out_ << " => ";
            if (lam->body) {
                if (lam->body->kind == Expr::Kind::Block) {
                    auto* blk = static_cast<const BlockExpr*>(lam->body);
                    out_ << "\n";
                    indent();
                    for (uint32_t i = 0; i < blk->stmt_count; ++i)
                        formatStmt(blk->stmts[i]);
                    if (blk->result) {
                        writeIndent();
                        formatExpr(blk->result);
                        out_ << "\n";
                    }
                    dedent();
                    writeIndent();
                } else {
                    formatExpr(lam->body);
                    out_ << " ";
                }
            }
            out_ << "}";
            break;
        }

        case Expr::Kind::MethodCall: {
            auto* mc = static_cast<const MethodCallExpr*>(expr);
            formatExpr(mc->object);
            out_ << "." << mc->method_name << "(";
            for (uint32_t i = 0; i < mc->arg_count; ++i) {
                if (i > 0) out_ << ", ";
                formatExpr(mc->args[i]);
            }
            out_ << ")";
            break;
        }

        case Expr::Kind::Try: {
            auto* te = static_cast<const TryExpr*>(expr);
            formatExpr(te->operand);
            out_ << "?";
            break;
        }
        case Expr::Kind::TupleLit: {
            auto* tl = static_cast<const TupleLitExpr*>(expr);
            out_ << "(";
            for (uint32_t i = 0; i < tl->count; ++i) {
                if (i > 0) out_ << ", ";
                formatExpr(tl->elements[i]);
            }
            out_ << ")";
            break;
        }
        case Expr::Kind::Uninit:
            out_ << "uninit";
            break;
        case Expr::Kind::ExprCall: {
            auto* ec = static_cast<const ExprCallExpr*>(expr);
            formatExpr(ec->callee);
            out_ << "(";
            for (uint32_t i = 0; i < ec->arg_count; ++i) {
                if (i > 0) out_ << ", ";
                formatExpr(ec->args[i]);
            }
            out_ << ")";
            break;
        }
    }
}

// ============================================================================
// Statement formatting
// ============================================================================

void Formatter::formatStmt(const Stmt* stmt) {
    writeIndent();
    switch (stmt->kind) {
        case Stmt::Kind::ValDecl: {
            auto* v = static_cast<const ValDeclStmt*>(stmt);
            out_ << "val " << v->name << ": ";
            formatTypeRef(v->type);
            out_ << " = ";
            formatExpr(v->init);
            break;
        }
        case Stmt::Kind::VarDecl: {
            auto* v = static_cast<const VarDeclStmt*>(stmt);
            out_ << "var " << v->name << ": ";
            formatTypeRef(v->type);
            out_ << " = ";
            formatExpr(v->init);
            break;
        }
        case Stmt::Kind::ExprStmt: {
            auto* e = static_cast<const ExprStmt*>(stmt);
            formatExpr(e->expr);
            break;
        }
        case Stmt::Kind::Assign: {
            auto* a = static_cast<const AssignStmt*>(stmt);
            out_ << a->name << " = ";
            formatExpr(a->value);
            break;
        }
        case Stmt::Kind::FieldAssign: {
            auto* f = static_cast<const FieldAssignStmt*>(stmt);
            formatExpr(f->target);
            out_ << " = ";
            formatExpr(f->value);
            break;
        }
        case Stmt::Kind::DerefAssign: {
            auto* d = static_cast<const DerefAssignStmt*>(stmt);
            formatExpr(d->target);
            out_ << " = ";
            formatExpr(d->value);
            break;
        }
        case Stmt::Kind::Break: {
            auto* b = static_cast<const BreakStmt*>(stmt);
            out_ << "break";
            if (b->value) {
                out_ << " ";
                formatExpr(b->value);
            }
            break;
        }
        case Stmt::Kind::Continue: {
            auto* c = static_cast<const ContinueStmt*>(stmt);
            out_ << "continue";
            if (c->arg_count > 0) {
                out_ << "(";
                for (uint32_t i = 0; i < c->arg_count; ++i) {
                    if (i > 0) out_ << ", ";
                    formatExpr(c->args[i]);
                }
                out_ << ")";
            }
            break;
        }
        case Stmt::Kind::IndexAssign: {
            auto* ia = static_cast<const IndexAssignStmt*>(stmt);
            formatExpr(ia->array);
            out_ << "[";
            formatExpr(ia->index);
            out_ << "] = ";
            formatExpr(ia->value);
            break;
        }
        case Stmt::Kind::TupleDestruct: {
            auto* td = static_cast<const TupleDestructStmt*>(stmt);
            out_ << (td->is_var ? "var (" : "val (");
            for (uint32_t i = 0; i < td->name_count; ++i) {
                if (i > 0) out_ << ", ";
                out_ << td->names[i];
            }
            out_ << ") = ";
            formatExpr(td->init);
            break;
        }
        case Stmt::Kind::Defer: {
            auto* ds = static_cast<const DeferStmt*>(stmt);
            out_ << "defer ";
            formatExpr(ds->body);
            break;
        }
    }
    out_ << "\n";
}

// ============================================================================
// Block formatting
// ============================================================================

void Formatter::formatBlock(const BlockExpr* block, bool /*is_fn_body*/) {
    out_ << "{\n";
    indent();
    for (uint32_t i = 0; i < block->stmt_count; ++i) {
        formatStmt(block->stmts[i]);
    }
    if (block->result) {
        writeIndent();
        formatExpr(block->result);
        out_ << "\n";
    }
    dedent();
    writeIndent();
    out_ << "}\n";
}

// ============================================================================
// Pattern formatting
// ============================================================================

void Formatter::formatPattern(const Pattern* pat) {
    switch (pat->kind) {
        case Pattern::Kind::IntLit:
            out_ << static_cast<const IntLitPattern*>(pat)->value;
            break;
        case Pattern::Kind::BoolLit:
            out_ << (static_cast<const BoolLitPattern*>(pat)->value ? "true" : "false");
            break;
        case Pattern::Kind::Wildcard:
            out_ << "_";
            break;
        case Pattern::Kind::Variable:
            out_ << static_cast<const VariablePattern*>(pat)->name;
            break;
        case Pattern::Kind::Enum:
            out_ << static_cast<const EnumPattern*>(pat)->variant_name;
            break;
        case Pattern::Kind::Range: {
            auto* r = static_cast<const RangePattern*>(pat);
            out_ << r->lo << (r->inclusive ? "..=" : "..") << r->hi;
            break;
        }
        case Pattern::Kind::Union: {
            auto* u = static_cast<const UnionPattern*>(pat);
            out_ << u->variant_name;
            if (u->inner) {
                out_ << "(";
                formatPattern(u->inner);
                out_ << ")";
            } else if (u->field_binding_count > 0) {
                out_ << "(";
                for (uint32_t i = 0; i < u->field_binding_count; ++i) {
                    if (i > 0) out_ << ", ";
                    out_ << u->field_bindings[i].field_name;
                    if (u->field_bindings[i].binding_name != u->field_bindings[i].field_name) {
                        out_ << ": " << u->field_bindings[i].binding_name;
                    }
                }
                out_ << ")";
            }
            break;
        }
    }
}

void Formatter::formatMatchArm(const MatchArm& arm) {
    writeIndent();
    formatPattern(arm.pattern);
    if (arm.guard) {
        out_ << " if ";
        formatExpr(arm.guard);
    }
    out_ << " => ";
    if (arm.body->kind == Expr::Kind::Block) {
        formatBlock(static_cast<const BlockExpr*>(arm.body));
    } else {
        formatExpr(arm.body);
        out_ << "\n";
    }
}

// ============================================================================
// Helpers
// ============================================================================

void Formatter::newline() {
    out_ << "\n";
}

void Formatter::writeIndent() {
    for (int i = 0; i < indent_; ++i) out_ << ' ';
}

const char* Formatter::binOpStr(BinOpKind op) {
    switch (op) {
        case BinOpKind::Add: return "+";
        case BinOpKind::Sub: return "-";
        case BinOpKind::Mul: return "*";
        case BinOpKind::Div: return "/";
        case BinOpKind::Mod: return "%";
        case BinOpKind::Eq: return "==";
        case BinOpKind::NotEq: return "!=";
        case BinOpKind::Lt: return "<";
        case BinOpKind::LtEq: return "<=";
        case BinOpKind::Gt: return ">";
        case BinOpKind::GtEq: return ">=";
        case BinOpKind::And: return "and";
        case BinOpKind::Or: return "or";
        case BinOpKind::BitAnd: return "&";
        case BinOpKind::BitOr: return "|";
        case BinOpKind::BitXor: return "^";
        case BinOpKind::Shl: return "<<";
        case BinOpKind::Shr: return ">>";
        case BinOpKind::AddWrap: return "+%";
        case BinOpKind::SubWrap: return "-%";
        case BinOpKind::MulWrap: return "*%";
        case BinOpKind::AddSat: return "+|";
        case BinOpKind::SubSat: return "-|";
    }
    return "??";
}

} // namespace kern
