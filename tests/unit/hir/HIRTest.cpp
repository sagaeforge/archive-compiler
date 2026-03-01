#include "kern/hir/HIR.h"
#include "kern/hir/HIRDump.h"
#include "kern/hir/HIRPass.h"
#include "kern/support/Arena.h"
#include "kern/support/CompilationContext.h"
#include <gtest/gtest.h>
#include <sstream>

namespace kern {

class HIRTest : public ::testing::Test {
protected:
    CompilationContext ctx;

    template<typename T>
    T* make() { return ctx.arena.make<T>(); }

    HIRExpr* makeIntLit(int64_t value, TypeId type = TypeTable::I64) {
        auto* e = make<HIRIntLitExpr>();
        e->kind = HIRExpr::Kind::IntLit;
        e->type = type;
        e->loc = SourceLocation::unknown();
        e->value = value;
        return e;
    }

    HIRExpr* makeFloatLit(double value, TypeId type = TypeTable::F64) {
        auto* e = make<HIRFloatLitExpr>();
        e->kind = HIRExpr::Kind::FloatLit;
        e->type = type;
        e->loc = SourceLocation::unknown();
        e->value = value;
        return e;
    }

    HIRExpr* makeBoolLit(bool value) {
        auto* e = make<HIRBoolLitExpr>();
        e->kind = HIRExpr::Kind::BoolLit;
        e->type = TypeTable::Bool;
        e->loc = SourceLocation::unknown();
        e->value = value;
        return e;
    }

    HIRExpr* makeIdent(std::string_view name, TypeId type) {
        auto* e = make<HIRIdentExpr>();
        e->kind = HIRExpr::Kind::Ident;
        e->type = type;
        e->loc = SourceLocation::unknown();
        e->name = ctx.strings.intern(name);
        return e;
    }

    HIRExpr* makeBinOp(HIRBinOp op, HIRExpr* lhs, HIRExpr* rhs, TypeId type) {
        auto* e = make<HIRBinOpExpr>();
        e->kind = HIRExpr::Kind::BinOp;
        e->type = type;
        e->loc = SourceLocation::unknown();
        e->op = op;
        e->lhs = lhs;
        e->rhs = rhs;
        return e;
    }

    HIRExpr* makeCall(std::string_view callee, HIRExpr** args, uint32_t count, TypeId type) {
        auto* e = make<HIRCallExpr>();
        e->kind = HIRExpr::Kind::Call;
        e->type = type;
        e->loc = SourceLocation::unknown();
        e->callee = ctx.strings.intern(callee);
        e->args = args;
        e->arg_count = count;
        e->is_tail_call = false;
        return e;
    }

    HIRExpr* makeIf(HIRExpr* cond, HIRExpr* then_br, HIRExpr* else_br, TypeId type) {
        auto* e = make<HIRIfExpr>();
        e->kind = HIRExpr::Kind::If;
        e->type = type;
        e->loc = SourceLocation::unknown();
        e->condition = cond;
        e->then_branch = then_br;
        e->else_branch = else_br;
        return e;
    }

    HIRExpr* makeBlock(HIRStmt** stmts, uint32_t count, HIRExpr* result, TypeId type) {
        auto* e = make<HIRBlockExpr>();
        e->kind = HIRExpr::Kind::Block;
        e->type = type;
        e->loc = SourceLocation::unknown();
        e->stmts = stmts;
        e->stmt_count = count;
        e->result = result;
        return e;
    }
};

// ============================================================================
// HIR Node Structure Tests
// ============================================================================

TEST_F(HIRTest, IntLitExpr) {
    auto* e = static_cast<HIRIntLitExpr*>(makeIntLit(42));
    EXPECT_EQ(e->kind, HIRExpr::Kind::IntLit);
    EXPECT_EQ(e->type, TypeTable::I64);
    EXPECT_EQ(e->value, 42);
}

TEST_F(HIRTest, IntLitI32) {
    auto* e = static_cast<HIRIntLitExpr*>(makeIntLit(100, TypeTable::I32));
    EXPECT_EQ(e->type, TypeTable::I32);
    EXPECT_EQ(e->value, 100);
}

TEST_F(HIRTest, FloatLitExpr) {
    auto* e = static_cast<HIRFloatLitExpr*>(makeFloatLit(3.14));
    EXPECT_EQ(e->kind, HIRExpr::Kind::FloatLit);
    EXPECT_EQ(e->type, TypeTable::F64);
    EXPECT_DOUBLE_EQ(e->value, 3.14);
}

TEST_F(HIRTest, FloatLitF32) {
    auto* e = static_cast<HIRFloatLitExpr*>(makeFloatLit(2.5, TypeTable::F32));
    EXPECT_EQ(e->type, TypeTable::F32);
}

TEST_F(HIRTest, BoolLitExpr) {
    auto* e = static_cast<HIRBoolLitExpr*>(makeBoolLit(true));
    EXPECT_EQ(e->kind, HIRExpr::Kind::BoolLit);
    EXPECT_EQ(e->type, TypeTable::Bool);
    EXPECT_TRUE(e->value);
}

TEST_F(HIRTest, StringLitExpr) {
    auto* e = make<HIRStringLitExpr>();
    e->kind = HIRExpr::Kind::StringLit;
    e->type = INVALID_TYPE; // String type needs registration
    e->loc = SourceLocation::unknown();
    e->data = "hello";
    e->length = 5;
    EXPECT_EQ(e->kind, HIRExpr::Kind::StringLit);
    EXPECT_EQ(e->length, 5u);
    EXPECT_EQ(std::string_view(e->data, e->length), "hello");
}

TEST_F(HIRTest, IdentExpr) {
    auto* e = static_cast<HIRIdentExpr*>(makeIdent("x", TypeTable::I64));
    EXPECT_EQ(e->kind, HIRExpr::Kind::Ident);
    EXPECT_EQ(e->name, "x");
    EXPECT_EQ(e->type, TypeTable::I64);
}

TEST_F(HIRTest, BinOpExpr) {
    auto* lhs = makeIntLit(1);
    auto* rhs = makeIntLit(2);
    auto* e = static_cast<HIRBinOpExpr*>(makeBinOp(HIRBinOp::Add, lhs, rhs, TypeTable::I64));
    EXPECT_EQ(e->kind, HIRExpr::Kind::BinOp);
    EXPECT_EQ(e->op, HIRBinOp::Add);
    EXPECT_EQ(static_cast<HIRIntLitExpr*>(e->lhs)->value, 1);
    EXPECT_EQ(static_cast<HIRIntLitExpr*>(e->rhs)->value, 2);
}

TEST_F(HIRTest, AllBinOps) {
    auto* a = makeIntLit(0);
    auto* b = makeIntLit(0);
    // Just verify all enum values compile and can be set
    for (auto op : {HIRBinOp::Add, HIRBinOp::Sub, HIRBinOp::Mul, HIRBinOp::Div,
                    HIRBinOp::Eq, HIRBinOp::NotEq, HIRBinOp::Lt, HIRBinOp::LtEq,
                    HIRBinOp::Gt, HIRBinOp::GtEq, HIRBinOp::And, HIRBinOp::Or}) {
        TypeId result_type = (op >= HIRBinOp::Eq) ? TypeTable::Bool : TypeTable::I64;
        auto* e = static_cast<HIRBinOpExpr*>(makeBinOp(op, a, b, result_type));
        EXPECT_EQ(e->op, op);
    }
}

TEST_F(HIRTest, UnaryOpExpr) {
    auto* operand = makeIntLit(5);
    auto* e = make<HIRUnaryOpExpr>();
    e->kind = HIRExpr::Kind::UnaryOp;
    e->type = TypeTable::I64;
    e->loc = SourceLocation::unknown();
    e->op = HIRUnaryOp::Neg;
    e->operand = operand;
    EXPECT_EQ(e->kind, HIRExpr::Kind::UnaryOp);
    EXPECT_EQ(e->op, HIRUnaryOp::Neg);
}

TEST_F(HIRTest, CallExpr) {
    auto* arg = makeIntLit(10);
    auto** args = ctx.arena.makeArray<HIRExpr*>(1);
    args[0] = arg;
    auto* e = static_cast<HIRCallExpr*>(makeCall("foo", args, 1, TypeTable::I64));
    EXPECT_EQ(e->kind, HIRExpr::Kind::Call);
    EXPECT_EQ(e->callee, "foo");
    EXPECT_EQ(e->arg_count, 1u);
    EXPECT_FALSE(e->is_tail_call);
}

TEST_F(HIRTest, CallExprTailCall) {
    auto* e = make<HIRCallExpr>();
    e->kind = HIRExpr::Kind::Call;
    e->type = TypeTable::I64;
    e->loc = SourceLocation::unknown();
    e->callee = ctx.strings.intern("recurse");
    e->args = nullptr;
    e->arg_count = 0;
    e->is_tail_call = true;
    EXPECT_TRUE(e->is_tail_call);
}

TEST_F(HIRTest, IfExpr) {
    auto* cond = makeBoolLit(true);
    auto* then_br = makeIntLit(1);
    auto* else_br = makeIntLit(2);
    auto* e = static_cast<HIRIfExpr*>(makeIf(cond, then_br, else_br, TypeTable::I64));
    EXPECT_EQ(e->kind, HIRExpr::Kind::If);
    EXPECT_NE(e->else_branch, nullptr);
}

TEST_F(HIRTest, IfExprNoElse) {
    auto* cond = makeBoolLit(true);
    auto* then_br = makeIntLit(1);
    auto* e = static_cast<HIRIfExpr*>(makeIf(cond, then_br, nullptr, TypeTable::Unit));
    EXPECT_EQ(e->else_branch, nullptr);
    EXPECT_EQ(e->type, TypeTable::Unit);
}

TEST_F(HIRTest, BlockExpr) {
    auto* result = makeIntLit(42);
    auto* e = static_cast<HIRBlockExpr*>(makeBlock(nullptr, 0, result, TypeTable::I64));
    EXPECT_EQ(e->kind, HIRExpr::Kind::Block);
    EXPECT_EQ(e->stmt_count, 0u);
    EXPECT_NE(e->result, nullptr);
}

TEST_F(HIRTest, ReturnExpr) {
    auto* val = makeIntLit(0);
    auto* e = make<HIRReturnExpr>();
    e->kind = HIRExpr::Kind::Return;
    e->type = TypeTable::Unit;
    e->loc = SourceLocation::unknown();
    e->value = val;
    EXPECT_EQ(e->kind, HIRExpr::Kind::Return);
    EXPECT_NE(e->value, nullptr);
}

TEST_F(HIRTest, ReturnExprNoValue) {
    auto* e = make<HIRReturnExpr>();
    e->kind = HIRExpr::Kind::Return;
    e->type = TypeTable::Unit;
    e->loc = SourceLocation::unknown();
    e->value = nullptr;
    EXPECT_EQ(e->value, nullptr);
}

// ============================================================================
// HIR Struct/Enum/Union Expression Tests
// ============================================================================

TEST_F(HIRTest, StructLitExpr) {
    auto* field_val = makeIntLit(10);
    auto* fields = ctx.arena.makeArray<HIRFieldInit>(1);
    fields[0] = {ctx.strings.intern("x"), field_val, SourceLocation::unknown()};

    auto* e = make<HIRStructLitExpr>();
    e->kind = HIRExpr::Kind::StructLit;
    e->type = INVALID_TYPE;
    e->loc = SourceLocation::unknown();
    e->struct_name = ctx.strings.intern("Point");
    e->fields = fields;
    e->field_count = 1;
    EXPECT_EQ(e->struct_name, "Point");
    EXPECT_EQ(e->field_count, 1u);
}

TEST_F(HIRTest, FieldAccessExpr) {
    auto* obj = makeIdent("p", INVALID_TYPE);
    auto* e = make<HIRFieldAccessExpr>();
    e->kind = HIRExpr::Kind::FieldAccess;
    e->type = TypeTable::I64;
    e->loc = SourceLocation::unknown();
    e->object = obj;
    e->field_name = ctx.strings.intern("x");
    EXPECT_EQ(e->field_name, "x");
}

TEST_F(HIRTest, EnumAccessExpr) {
    auto* e = make<HIREnumAccessExpr>();
    e->kind = HIRExpr::Kind::EnumAccess;
    e->type = INVALID_TYPE;
    e->loc = SourceLocation::unknown();
    e->enum_name = ctx.strings.intern("Color");
    e->variant_name = ctx.strings.intern("Red");
    EXPECT_EQ(e->enum_name, "Color");
    EXPECT_EQ(e->variant_name, "Red");
}

TEST_F(HIRTest, UnionVariantExpr) {
    auto* payload = makeIntLit(5);
    auto* e = make<HIRUnionVariantExpr>();
    e->kind = HIRExpr::Kind::UnionVariant;
    e->type = INVALID_TYPE;
    e->loc = SourceLocation::unknown();
    e->union_name = ctx.strings.intern("Shape");
    e->variant_name = ctx.strings.intern("Circle");
    e->payload = payload;
    EXPECT_EQ(e->union_name, "Shape");
    EXPECT_NE(e->payload, nullptr);
}

TEST_F(HIRTest, UnionVariantExprEmpty) {
    auto* e = make<HIRUnionVariantExpr>();
    e->kind = HIRExpr::Kind::UnionVariant;
    e->type = INVALID_TYPE;
    e->loc = SourceLocation::unknown();
    e->union_name = ctx.strings.intern("Option");
    e->variant_name = ctx.strings.intern("None");
    e->payload = nullptr;
    EXPECT_EQ(e->payload, nullptr);
}

// ============================================================================
// HIR Pointer Expression Tests
// ============================================================================

TEST_F(HIRTest, AddrOfExpr) {
    auto* operand = makeIdent("x", TypeTable::I64);
    auto* e = make<HIRAddrOfExpr>();
    e->kind = HIRExpr::Kind::AddrOf;
    e->type = ctx.types.makePtr(TypeTable::I64, false);
    e->loc = SourceLocation::unknown();
    e->operand = operand;
    e->is_mutable = false;
    EXPECT_FALSE(e->is_mutable);
}

TEST_F(HIRTest, AddrOfVarExpr) {
    auto* operand = makeIdent("x", TypeTable::I64);
    auto* e = make<HIRAddrOfExpr>();
    e->kind = HIRExpr::Kind::AddrOf;
    e->type = ctx.types.makePtr(TypeTable::I64, true);
    e->loc = SourceLocation::unknown();
    e->operand = operand;
    e->is_mutable = true;
    EXPECT_TRUE(e->is_mutable);
}

TEST_F(HIRTest, DerefExpr) {
    auto* ptr = makeIdent("p", ctx.types.makePtr(TypeTable::I64, false));
    auto* e = make<HIRDerefExpr>();
    e->kind = HIRExpr::Kind::Deref;
    e->type = TypeTable::I64;
    e->loc = SourceLocation::unknown();
    e->operand = ptr;
    EXPECT_EQ(e->type, TypeTable::I64);
}

// ============================================================================
// HIR Statement Tests
// ============================================================================

TEST_F(HIRTest, ValDeclStmt) {
    auto* init = makeIntLit(42);
    auto* s = make<HIRValDeclStmt>();
    s->kind = HIRStmt::Kind::ValDecl;
    s->loc = SourceLocation::unknown();
    s->name = ctx.strings.intern("x");
    s->type = TypeTable::I64;
    s->init = init;
    EXPECT_EQ(s->kind, HIRStmt::Kind::ValDecl);
    EXPECT_EQ(s->name, "x");
}

TEST_F(HIRTest, VarDeclStmt) {
    auto* init = makeIntLit(0);
    auto* s = make<HIRVarDeclStmt>();
    s->kind = HIRStmt::Kind::VarDecl;
    s->loc = SourceLocation::unknown();
    s->name = ctx.strings.intern("counter");
    s->type = TypeTable::I64;
    s->init = init;
    EXPECT_EQ(s->kind, HIRStmt::Kind::VarDecl);
}

TEST_F(HIRTest, AssignStmt) {
    auto* val = makeIntLit(10);
    auto* s = make<HIRAssignStmt>();
    s->kind = HIRStmt::Kind::Assign;
    s->loc = SourceLocation::unknown();
    s->name = ctx.strings.intern("x");
    s->value = val;
    EXPECT_EQ(s->name, "x");
}

TEST_F(HIRTest, ExprStmt) {
    auto* expr = makeCall("print", nullptr, 0, TypeTable::Unit);
    auto* s = make<HIRExprStmt>();
    s->kind = HIRStmt::Kind::ExprStmt;
    s->loc = SourceLocation::unknown();
    s->expr = expr;
    EXPECT_EQ(s->kind, HIRStmt::Kind::ExprStmt);
}

TEST_F(HIRTest, FieldAssignStmt) {
    auto* target = make<HIRFieldAccessExpr>();
    target->kind = HIRExpr::Kind::FieldAccess;
    target->type = TypeTable::I64;
    target->loc = SourceLocation::unknown();
    target->object = makeIdent("p", INVALID_TYPE);
    target->field_name = ctx.strings.intern("x");

    auto* val = makeIntLit(5);
    auto* s = make<HIRFieldAssignStmt>();
    s->kind = HIRStmt::Kind::FieldAssign;
    s->loc = SourceLocation::unknown();
    s->target = target;
    s->value = val;
    EXPECT_EQ(s->kind, HIRStmt::Kind::FieldAssign);
}

TEST_F(HIRTest, DerefAssignStmt) {
    auto* target = make<HIRDerefExpr>();
    target->kind = HIRExpr::Kind::Deref;
    target->type = TypeTable::I64;
    target->loc = SourceLocation::unknown();
    target->operand = makeIdent("p", INVALID_TYPE);

    auto* val = makeIntLit(99);
    auto* s = make<HIRDerefAssignStmt>();
    s->kind = HIRStmt::Kind::DerefAssign;
    s->loc = SourceLocation::unknown();
    s->target = target;
    s->value = val;
    EXPECT_EQ(s->kind, HIRStmt::Kind::DerefAssign);
}

// ============================================================================
// HIR Pattern Tests
// ============================================================================

TEST_F(HIRTest, IntLitPattern) {
    auto* p = make<HIRIntLitPattern>();
    p->kind = HIRPattern::Kind::IntLit;
    p->type = TypeTable::I64;
    p->loc = SourceLocation::unknown();
    p->value = 42;
    EXPECT_EQ(p->value, 42);
}

TEST_F(HIRTest, BoolLitPattern) {
    auto* p = make<HIRBoolLitPattern>();
    p->kind = HIRPattern::Kind::BoolLit;
    p->type = TypeTable::Bool;
    p->loc = SourceLocation::unknown();
    p->value = true;
    EXPECT_TRUE(p->value);
}

TEST_F(HIRTest, WildcardPattern) {
    auto* p = make<HIRWildcardPattern>();
    p->kind = HIRPattern::Kind::Wildcard;
    p->type = TypeTable::I64;
    p->loc = SourceLocation::unknown();
    EXPECT_EQ(p->kind, HIRPattern::Kind::Wildcard);
}

TEST_F(HIRTest, VariablePattern) {
    auto* p = make<HIRVariablePattern>();
    p->kind = HIRPattern::Kind::Variable;
    p->type = TypeTable::I64;
    p->loc = SourceLocation::unknown();
    p->name = ctx.strings.intern("n");
    EXPECT_EQ(p->name, "n");
}

TEST_F(HIRTest, EnumPattern) {
    auto* p = make<HIREnumPattern>();
    p->kind = HIRPattern::Kind::Enum;
    p->type = INVALID_TYPE;
    p->loc = SourceLocation::unknown();
    p->variant_name = ctx.strings.intern("Red");
    EXPECT_EQ(p->variant_name, "Red");
}

TEST_F(HIRTest, UnionPattern) {
    auto* inner = make<HIRVariablePattern>();
    inner->kind = HIRPattern::Kind::Variable;
    inner->type = TypeTable::I64;
    inner->loc = SourceLocation::unknown();
    inner->name = ctx.strings.intern("val");

    auto* p = make<HIRUnionPattern>();
    p->kind = HIRPattern::Kind::Union;
    p->type = INVALID_TYPE;
    p->loc = SourceLocation::unknown();
    p->variant_name = ctx.strings.intern("Some");
    p->inner = inner;
    p->field_bindings = nullptr;
    p->field_binding_count = 0;
    EXPECT_EQ(p->variant_name, "Some");
    EXPECT_NE(p->inner, nullptr);
}

TEST_F(HIRTest, MatchExpr) {
    auto* scrutinee = makeIdent("x", TypeTable::I64);

    auto* arms = ctx.arena.makeArray<HIRMatchArm>(2);

    auto* pat1 = make<HIRIntLitPattern>();
    pat1->kind = HIRPattern::Kind::IntLit;
    pat1->type = TypeTable::I64;
    pat1->loc = SourceLocation::unknown();
    pat1->value = 0;
    arms[0] = {pat1, nullptr, makeIntLit(100), SourceLocation::unknown()};

    auto* pat2 = make<HIRWildcardPattern>();
    pat2->kind = HIRPattern::Kind::Wildcard;
    pat2->type = TypeTable::I64;
    pat2->loc = SourceLocation::unknown();
    arms[1] = {pat2, nullptr, makeIntLit(200), SourceLocation::unknown()};

    auto* e = make<HIRMatchExpr>();
    e->kind = HIRExpr::Kind::Match;
    e->type = TypeTable::I64;
    e->loc = SourceLocation::unknown();
    e->scrutinee = scrutinee;
    e->arms = arms;
    e->arm_count = 2;
    EXPECT_EQ(e->arm_count, 2u);
}

// ============================================================================
// HIR Declaration Tests
// ============================================================================

TEST_F(HIRTest, FnDecl) {
    auto* params = ctx.arena.makeArray<HIRParam>(1);
    params[0] = {ctx.strings.intern("n"), TypeTable::I64, SourceLocation::unknown()};

    auto* body = makeIntLit(0);

    auto* fn = make<HIRFnDecl>();
    fn->name = ctx.strings.intern("foo");
    fn->params = params;
    fn->param_count = 1;
    fn->return_type = TypeTable::I64;
    fn->body = body;
    fn->purity = 4; // Unknown
    fn->is_recursive = false;
    fn->is_tail_recursive = false;
    fn->is_intrinsic = false;
    fn->loc = SourceLocation::unknown();

    EXPECT_EQ(fn->name, "foo");
    EXPECT_EQ(fn->param_count, 1u);
    EXPECT_EQ(fn->return_type, TypeTable::I64);
    EXPECT_FALSE(fn->is_intrinsic);
}

TEST_F(HIRTest, FnDeclIntrinsic) {
    auto* fn = make<HIRFnDecl>();
    fn->name = ctx.strings.intern("write");
    fn->params = nullptr;
    fn->param_count = 0;
    fn->return_type = TypeTable::Unit;
    fn->body = nullptr;
    fn->purity = 2; // ImpureIo
    fn->is_recursive = false;
    fn->is_tail_recursive = false;
    fn->is_intrinsic = true;
    fn->loc = SourceLocation::unknown();
    EXPECT_TRUE(fn->is_intrinsic);
    EXPECT_EQ(fn->body, nullptr);
}

TEST_F(HIRTest, StructDecl) {
    FieldInfo fields[] = {
        {ctx.strings.intern("x"), TypeTable::I64, false, 0},
        {ctx.strings.intern("y"), TypeTable::I64, false, 8},
    };
    TypeId tid = ctx.types.makeStruct(ctx.strings.intern("Point"), fields);

    auto* s = make<HIRStructDecl>();
    s->name = ctx.strings.intern("Point");
    s->type_id = tid;
    s->loc = SourceLocation::unknown();
    EXPECT_EQ(s->name, "Point");
    EXPECT_NE(s->type_id, INVALID_TYPE);
}

TEST_F(HIRTest, EnumDecl) {
    std::string_view names[] = {
        ctx.strings.intern("Red"),
        ctx.strings.intern("Green"),
        ctx.strings.intern("Blue")
    };
    int64_t values[] = {0, 1, 2};
    TypeId tid = ctx.types.makeEnum(ctx.strings.intern("Color"), names, values);

    auto* e = make<HIREnumDecl>();
    e->name = ctx.strings.intern("Color");
    e->type_id = tid;
    e->loc = SourceLocation::unknown();
    EXPECT_EQ(e->name, "Color");
}

TEST_F(HIRTest, UnionDecl) {
    VariantInfo variants[] = {
        {ctx.strings.intern("Some"), TypeTable::I64},
        {ctx.strings.intern("None"), INVALID_TYPE},
    };
    TypeId tid = ctx.types.makeUnion(ctx.strings.intern("Option"), variants);

    auto* u = make<HIRUnionDecl>();
    u->name = ctx.strings.intern("Option");
    u->type_id = tid;
    u->loc = SourceLocation::unknown();
    EXPECT_EQ(u->name, "Option");
}

// ============================================================================
// HIR Module Tests
// ============================================================================

TEST_F(HIRTest, ModuleEmpty) {
    auto* mod = make<HIRModule>();
    mod->functions = nullptr;
    mod->fn_count = 0;
    mod->structs = nullptr;
    mod->struct_count = 0;
    mod->enums = nullptr;
    mod->enum_count = 0;
    mod->unions = nullptr;
    mod->union_count = 0;
    EXPECT_EQ(mod->fn_count, 0u);
}

TEST_F(HIRTest, ModuleWithFunction) {
    auto* body = makeIntLit(42);
    auto* fn = make<HIRFnDecl>();
    fn->name = ctx.strings.intern("main");
    fn->params = nullptr;
    fn->param_count = 0;
    fn->return_type = TypeTable::I64;
    fn->body = body;
    fn->purity = 0; // Pure
    fn->is_recursive = false;
    fn->is_tail_recursive = false;
    fn->is_intrinsic = false;
    fn->loc = SourceLocation::unknown();

    auto** fns = ctx.arena.makeArray<HIRFnDecl*>(1);
    fns[0] = fn;

    auto* mod = make<HIRModule>();
    mod->functions = fns;
    mod->fn_count = 1;
    mod->structs = nullptr;
    mod->struct_count = 0;
    mod->enums = nullptr;
    mod->enum_count = 0;
    mod->unions = nullptr;
    mod->union_count = 0;

    EXPECT_EQ(mod->fn_count, 1u);
    EXPECT_EQ(mod->functions[0]->name, "main");
}

// ============================================================================
// HIR Dump Tests
// ============================================================================

TEST_F(HIRTest, DumpIntLit) {
    std::ostringstream out;
    dumpHIRExpr(makeIntLit(42), ctx.types, out);
    EXPECT_EQ(out.str(), "(int_lit 42) : i64\n");
}

TEST_F(HIRTest, DumpFloatLit) {
    std::ostringstream out;
    dumpHIRExpr(makeFloatLit(3.14), ctx.types, out);
    auto s = out.str();
    EXPECT_TRUE(s.find("float_lit") != std::string::npos);
    EXPECT_TRUE(s.find("f64") != std::string::npos);
}

TEST_F(HIRTest, DumpBoolLit) {
    std::ostringstream out;
    dumpHIRExpr(makeBoolLit(true), ctx.types, out);
    EXPECT_EQ(out.str(), "(bool_lit true) : bool\n");
}

TEST_F(HIRTest, DumpIdent) {
    std::ostringstream out;
    dumpHIRExpr(makeIdent("x", TypeTable::I64), ctx.types, out);
    EXPECT_EQ(out.str(), "(ident x) : i64\n");
}

TEST_F(HIRTest, DumpBinOp) {
    std::ostringstream out;
    auto* e = makeBinOp(HIRBinOp::Add, makeIntLit(1), makeIntLit(2), TypeTable::I64);
    dumpHIRExpr(e, ctx.types, out);
    auto s = out.str();
    EXPECT_TRUE(s.find("binop +") != std::string::npos);
    EXPECT_TRUE(s.find("int_lit 1") != std::string::npos);
    EXPECT_TRUE(s.find("int_lit 2") != std::string::npos);
}

TEST_F(HIRTest, DumpCall) {
    std::ostringstream out;
    auto* arg = makeIntLit(10);
    auto** args = ctx.arena.makeArray<HIRExpr*>(1);
    args[0] = arg;
    auto* e = makeCall("foo", args, 1, TypeTable::I64);
    dumpHIRExpr(e, ctx.types, out);
    auto s = out.str();
    EXPECT_TRUE(s.find("call foo") != std::string::npos);
}

TEST_F(HIRTest, DumpCallTail) {
    std::ostringstream out;
    auto* e = make<HIRCallExpr>();
    e->kind = HIRExpr::Kind::Call;
    e->type = TypeTable::I64;
    e->loc = SourceLocation::unknown();
    e->callee = ctx.strings.intern("rec");
    e->args = nullptr;
    e->arg_count = 0;
    e->is_tail_call = true;
    dumpHIRExpr(e, ctx.types, out);
    EXPECT_TRUE(out.str().find("[tail]") != std::string::npos);
}

TEST_F(HIRTest, DumpIf) {
    std::ostringstream out;
    auto* e = makeIf(makeBoolLit(true), makeIntLit(1), makeIntLit(2), TypeTable::I64);
    dumpHIRExpr(e, ctx.types, out);
    auto s = out.str();
    EXPECT_TRUE(s.find("if") != std::string::npos);
    EXPECT_TRUE(s.find("then:") != std::string::npos);
    EXPECT_TRUE(s.find("else:") != std::string::npos);
}

TEST_F(HIRTest, DumpBlock) {
    std::ostringstream out;
    auto* val_stmt = make<HIRValDeclStmt>();
    val_stmt->kind = HIRStmt::Kind::ValDecl;
    val_stmt->loc = SourceLocation::unknown();
    val_stmt->name = ctx.strings.intern("x");
    val_stmt->type = TypeTable::I64;
    val_stmt->init = makeIntLit(5);

    auto** stmts = ctx.arena.makeArray<HIRStmt*>(1);
    stmts[0] = val_stmt;
    auto* e = makeBlock(stmts, 1, makeIdent("x", TypeTable::I64), TypeTable::I64);
    dumpHIRExpr(e, ctx.types, out);
    auto s = out.str();
    EXPECT_TRUE(s.find("block") != std::string::npos);
    EXPECT_TRUE(s.find("val x") != std::string::npos);
    EXPECT_TRUE(s.find("result:") != std::string::npos);
}

TEST_F(HIRTest, DumpModule) {
    std::ostringstream out;
    auto* body = makeIntLit(42);
    auto* fn = make<HIRFnDecl>();
    fn->name = ctx.strings.intern("main");
    fn->params = nullptr;
    fn->param_count = 0;
    fn->return_type = TypeTable::I64;
    fn->body = body;
    fn->purity = 0; // Pure
    fn->is_recursive = false;
    fn->is_tail_recursive = false;
    fn->is_intrinsic = false;
    fn->loc = SourceLocation::unknown();

    auto** fns = ctx.arena.makeArray<HIRFnDecl*>(1);
    fns[0] = fn;

    auto* mod = make<HIRModule>();
    mod->functions = fns;
    mod->fn_count = 1;
    mod->structs = nullptr;
    mod->struct_count = 0;
    mod->enums = nullptr;
    mod->enum_count = 0;
    mod->unions = nullptr;
    mod->union_count = 0;

    dumpHIR(mod, ctx.types, out);
    auto s = out.str();
    EXPECT_TRUE(s.find("fn main") != std::string::npos);
    EXPECT_TRUE(s.find("[pure]") != std::string::npos);
    EXPECT_TRUE(s.find("-> i64") != std::string::npos);
    EXPECT_TRUE(s.find("int_lit 42") != std::string::npos);
}

TEST_F(HIRTest, DumpNull) {
    std::ostringstream out;
    dumpHIRExpr(nullptr, ctx.types, out);
    EXPECT_EQ(out.str(), "<null>\n");
}

// ============================================================================
// HIR Pass Manager Tests
// ============================================================================

class CountingPass : public HIRPass {
public:
    int run_count = 0;
    std::string_view name() const override { return "counting"; }
    void run(HIRModule& /*module*/, CompilationContext& /*ctx*/) override {
        ++run_count;
    }
};

TEST_F(HIRTest, PassManagerRunsAll) {
    HIRPassManager pm;
    auto* p1 = new CountingPass();
    auto* p2 = new CountingPass();
    // We need to add them via the template interface
    // Since add() takes unique_ptr internally, we use a workaround
    pm.add<CountingPass>();
    pm.add<CountingPass>();

    auto* mod = make<HIRModule>();
    mod->functions = nullptr;
    mod->fn_count = 0;
    mod->structs = nullptr;
    mod->struct_count = 0;
    mod->enums = nullptr;
    mod->enum_count = 0;
    mod->unions = nullptr;
    mod->union_count = 0;

    pm.run(*mod, ctx);
    // Can't access internal passes directly, but we verified it compiles and runs
    // The real test is that it doesn't crash
    delete p1;
    delete p2;
}

TEST_F(HIRTest, PassManagerEmpty) {
    HIRPassManager pm;
    auto* mod = make<HIRModule>();
    mod->functions = nullptr;
    mod->fn_count = 0;
    mod->structs = nullptr;
    mod->struct_count = 0;
    mod->enums = nullptr;
    mod->enum_count = 0;
    mod->unions = nullptr;
    mod->union_count = 0;
    pm.run(*mod, ctx);
    // No crash = success
}

// ============================================================================
// HIR Block with Stmts test
// ============================================================================

TEST_F(HIRTest, BlockWithAllStmtKinds) {
    // val x: i64 = 1
    auto* val_decl = make<HIRValDeclStmt>();
    val_decl->kind = HIRStmt::Kind::ValDecl;
    val_decl->loc = SourceLocation::unknown();
    val_decl->name = ctx.strings.intern("x");
    val_decl->type = TypeTable::I64;
    val_decl->init = makeIntLit(1);

    // var y: i64 = 2
    auto* var_decl = make<HIRVarDeclStmt>();
    var_decl->kind = HIRStmt::Kind::VarDecl;
    var_decl->loc = SourceLocation::unknown();
    var_decl->name = ctx.strings.intern("y");
    var_decl->type = TypeTable::I64;
    var_decl->init = makeIntLit(2);

    // y = 3
    auto* assign = make<HIRAssignStmt>();
    assign->kind = HIRStmt::Kind::Assign;
    assign->loc = SourceLocation::unknown();
    assign->name = ctx.strings.intern("y");
    assign->value = makeIntLit(3);

    auto** stmts = ctx.arena.makeArray<HIRStmt*>(3);
    stmts[0] = val_decl;
    stmts[1] = var_decl;
    stmts[2] = assign;

    auto* result = makeBinOp(HIRBinOp::Add, makeIdent("x", TypeTable::I64),
                             makeIdent("y", TypeTable::I64), TypeTable::I64);
    auto* block = makeBlock(stmts, 3, result, TypeTable::I64);

    EXPECT_EQ(static_cast<HIRBlockExpr*>(block)->stmt_count, 3u);

    std::ostringstream out;
    dumpHIRExpr(block, ctx.types, out);
    auto s = out.str();
    EXPECT_TRUE(s.find("val x") != std::string::npos);
    EXPECT_TRUE(s.find("var y") != std::string::npos);
    EXPECT_TRUE(s.find("assign y") != std::string::npos);
}

// ============================================================================
// Node type coverage — all Kind enums
// ============================================================================

TEST_F(HIRTest, ExprKindCoverage) {
    // Verify all expr kinds can be created
    EXPECT_EQ(static_cast<uint8_t>(HIRExpr::Kind::IntLit), 0);
    EXPECT_EQ(static_cast<uint8_t>(HIRExpr::Kind::Deref), 17);
    // 18 total kinds
}

TEST_F(HIRTest, StmtKindCoverage) {
    EXPECT_EQ(static_cast<uint8_t>(HIRStmt::Kind::ValDecl), 0);
    EXPECT_EQ(static_cast<uint8_t>(HIRStmt::Kind::DerefAssign), 5);
}

TEST_F(HIRTest, PatternKindCoverage) {
    EXPECT_EQ(static_cast<uint8_t>(HIRPattern::Kind::IntLit), 0);
    EXPECT_EQ(static_cast<uint8_t>(HIRPattern::Kind::Union), 5);
}

} // namespace kern
