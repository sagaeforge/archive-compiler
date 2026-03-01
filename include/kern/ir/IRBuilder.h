#pragma once
#include "kern/ir/KernIR.h"
#include "kern/parser/AST.h"
#include <unordered_map>
#include <string_view>

namespace kern {

class IRBuilder {
public:
    IRModule build(Module* mod);

private:
    void buildFunction(FnDecl* fn);
    ValueId buildExpr(Expr* expr);
    void buildStmt(Stmt* stmt);

    ValueId emit(IRInstr instr);
    ValueId newValue();
    uint32_t newBlock(const std::string& label);
    void switchToBlock(uint32_t idx);

    IRModule module_;
    IRFunction* current_fn_ = nullptr;
    uint32_t current_block_ = 0;

    std::unordered_map<std::string_view, ValueId> locals_;
    uint32_t label_counter_ = 0;
};

} // namespace kern
