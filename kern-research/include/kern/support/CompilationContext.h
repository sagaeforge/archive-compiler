#pragma once
#include "kern/support/Arena.h"
#include "kern/support/Diagnostic.h"
#include "kern/support/StringPool.h"
#include "kern/support/TypeSystem.h"

namespace kern {

// CompilationContext holds all shared state for a single compilation.
// Passed by reference to every pipeline stage.
// Owns the Arena lifetime — all AST/HIR/LIR nodes live here.
struct CompilationContext {
    Arena arena;
    StringPool strings;
    TypeTable types;
    DiagnosticEngine diag;

    CompilationContext()
        : strings(arena), types(arena) {}

    // Non-copyable, non-movable (Arena owns allocated memory)
    CompilationContext(const CompilationContext&) = delete;
    CompilationContext& operator=(const CompilationContext&) = delete;
    CompilationContext(CompilationContext&&) = delete;
    CompilationContext& operator=(CompilationContext&&) = delete;
};

} // namespace kern
