#pragma once
#include "kern/ir/KernIR.h"
#include "kern/parser/AST.h"
#include "kern/sema/TypeChecker.h"
#include <unordered_map>
#include <string>
#include <string_view>

namespace kern {

struct IRStructInfo {
    std::string name;
    int32_t total_size;
    struct FieldInfo {
        std::string name;
        int32_t offset;
        IRType type;
        std::string struct_name; // for nested structs
    };
    std::vector<FieldInfo> fields;
};

class IRBuilder {
public:
    IRModule build(Module* mod, const TypeChecker& tc);

private:
    void buildFunction(FnDecl* fn);
    ValueId buildExpr(Expr* expr, bool in_tail_position = false);
    ValueId buildMatchChain(MatchExpr* matchE, ValueId scrutinee,
                            uint32_t arm_idx, bool in_tail_position);
    void buildStmt(Stmt* stmt);

    ValueId emit(IRInstr instr);
    ValueId newValue();
    uint32_t newBlock(const std::string& label);
    void switchToBlock(uint32_t idx);

    IRModule module_;
    IRFunction* current_fn_ = nullptr;
    uint32_t current_block_ = 0;
    const TypeChecker* tc_ = nullptr;

    void populateStructInfo();
    const IRStructInfo* getStructInfo(const std::string& name) const;

    std::unordered_map<std::string_view, ValueId> locals_;
    uint32_t label_counter_ = 0;
    std::unordered_map<std::string, IRStructInfo> struct_info_;
};

} // namespace kern
