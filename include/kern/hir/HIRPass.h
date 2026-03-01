#pragma once
#include "kern/hir/HIR.h"
#include <memory>
#include <string_view>
#include <vector>

namespace kern {

struct CompilationContext;

class HIRPass {
public:
    virtual ~HIRPass() = default;
    virtual std::string_view name() const = 0;
    virtual void run(HIRModule& module, CompilationContext& ctx) = 0;
};

class HIRPassManager {
public:
    template<typename T, typename... Args>
    void add(Args&&... args) {
        passes_.push_back(std::make_unique<T>(std::forward<Args>(args)...));
    }

    void run(HIRModule& module, CompilationContext& ctx) {
        for (auto& pass : passes_)
            pass->run(module, ctx);
    }

private:
    std::vector<std::unique_ptr<HIRPass>> passes_;
};

} // namespace kern
