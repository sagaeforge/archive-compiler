#pragma once
#include "kern/support/CompilationContext.h"
#include "kern/hir/HIR.h"
#include <string>
#include <string_view>
#include <unordered_map>

namespace kern {

struct Module;  // forward from parser

struct CachedFileState {
    std::string content;
    Module* ast = nullptr;
    HIRModule* hir = nullptr;
    uint64_t version = 0;
    bool dirty = true;
};

// IDE session: manages open files with lazy, cached AST→HIR rebuilds.
// Queries (completion, hover, goto) operate on the cached HIR.

class IDEContext {
public:
    explicit IDEContext(CompilationContext& ctx);

    // File lifecycle
    void openFile(std::string_view path, std::string_view content);
    void updateFile(std::string_view path, std::string_view content);
    void closeFile(std::string_view path);

    // Query API — lazy rebuild on access
    const HIRModule* getHIR(std::string_view path);
    const Module* getAST(std::string_view path);
    std::string_view getContent(std::string_view path) const;

    // File management
    bool hasFile(std::string_view path) const;
    size_t fileCount() const { return files_.size(); }
    uint64_t fileVersion(std::string_view path) const;

    CompilationContext& context() { return ctx_; }

private:
    void rebuildFile(CachedFileState& state);

    CompilationContext& ctx_;
    std::unordered_map<std::string, CachedFileState> files_;
};

} // namespace kern
