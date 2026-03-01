#pragma once
#include "kern/ir/KernIR.h"
#include "kern/parser/AST.h"
#include "kern/sema/TypeChecker.h"
#include <unordered_map>
#include <string_view>

namespace kern {

class IRBuilder {
public:
    IRModule build(Module* mod, const TypeChecker& tc);

private:
    void buildFunction(FnDecl* fn);
    ValueId buildExpr(Expr* expr, bool in_tail_position = false);
    void buildStmt(Stmt* stmt);

    ValueId emit(IRInstr instr);
    ValueId newValue();
    uint32_t newBlock(const std::string& label);
    void switchToBlock(uint32_t idx);

    IRModule module_;
    IRFunction* current_fn_ = nullptr;
    uint32_t current_block_ = 0;
    const TypeChecker* tc_ = nullptr;

    std::unordered_map<std::string_view, ValueId> locals_;
    uint32_t label_counter_ = 0;
};

} // namespace kern
