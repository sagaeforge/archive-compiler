#include "kern/ide/IDEContext.h"
#include "kern/lexer/Lexer.h"
#include "kern/parser/Parser.h"
#include "kern/hir/HIRBuilder.h"

namespace kern {

IDEContext::IDEContext(CompilationContext& ctx) : ctx_(ctx) {}

void IDEContext::openFile(std::string_view path, std::string_view content) {
    std::string key(path);
    auto& state = files_[key];
    state.content = std::string(content);
    state.version = 1;
    state.dirty = true;
    state.ast = nullptr;
    state.hir = nullptr;
}

void IDEContext::updateFile(std::string_view path, std::string_view content) {
    auto it = files_.find(std::string(path));
    if (it == files_.end()) {
        openFile(path, content);
        return;
    }
    it->second.content = std::string(content);
    it->second.version++;
    it->second.dirty = true;
    it->second.ast = nullptr;
    it->second.hir = nullptr;
}

void IDEContext::closeFile(std::string_view path) {
    files_.erase(std::string(path));
}

bool IDEContext::hasFile(std::string_view path) const {
    return files_.count(std::string(path)) > 0;
}

std::string_view IDEContext::getContent(std::string_view path) const {
    auto it = files_.find(std::string(path));
    if (it == files_.end()) return {};
    return it->second.content;
}

uint64_t IDEContext::fileVersion(std::string_view path) const {
    auto it = files_.find(std::string(path));
    if (it == files_.end()) return 0;
    return it->second.version;
}

const Module* IDEContext::getAST(std::string_view path) {
    auto it = files_.find(std::string(path));
    if (it == files_.end()) return nullptr;

    auto& state = it->second;
    if (state.dirty || state.ast == nullptr) {
        rebuildFile(state);
    }
    return state.ast;
}

const HIRModule* IDEContext::getHIR(std::string_view path) {
    auto it = files_.find(std::string(path));
    if (it == files_.end()) return nullptr;

    auto& state = it->second;
    if (state.dirty || state.hir == nullptr) {
        rebuildFile(state);
    }
    return state.hir;
}

void IDEContext::rebuildFile(CachedFileState& state) {
    // Parse
    Lexer lexer(state.content, "<ide>", ctx_.diag);
    Parser parser(lexer, ctx_.arena, ctx_.diag);
    state.ast = parser.parseModule();

    // Build HIR (type-check + desugar)
    if (state.ast && !ctx_.diag.hasErrors()) {
        HIRBuilder builder(ctx_);
        state.hir = builder.build(state.ast);
    } else {
        state.hir = nullptr;
    }

    state.dirty = false;
}

} // namespace kern
