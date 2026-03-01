#pragma once
#include "kern/parser/AST.h"
#include "kern/ir/Metadata.h"
#include "kern/support/Diagnostic.h"
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace kern {

struct PurityResult {
    Purity purity = Purity::Pure;
    bool is_recursive = false;
    bool uses_var = false;
};

class PurityChecker {
public:
    explicit PurityChecker(DiagnosticEngine& diag);

    // Analyze all functions in the module, returns function name -> purity mapping
    std::unordered_map<std::string_view, PurityResult> analyze(Module* mod);

private:
    // Build call graph: caller -> set of callees
    void buildCallGraph(Module* mod);

    // Check if a function body uses var
    bool bodyUsesVar(FnDecl* fn) const;

    // Collect all callees from an expression
    void collectCallees(Expr* expr, std::unordered_set<std::string_view>& callees) const;
    void collectCalleesStmt(Stmt* stmt, std::unordered_set<std::string_view>& callees) const;

    // Reverse topological sort (leaves first)
    std::vector<std::string_view> reverseTopoSort() const;

    // Detect recursive functions
    bool isRecursive(std::string_view fn_name) const;

    DiagnosticEngine& diag_;

    // fn_name -> FnDecl*
    std::unordered_map<std::string_view, FnDecl*> fn_map_;

    // Call graph: fn_name -> set of callees
    std::unordered_map<std::string_view, std::unordered_set<std::string_view>> call_graph_;
};

} // namespace kern
