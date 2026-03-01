#pragma once
#include "kern/lir/LIR.h"
#include <memory>
#include <string_view>
#include <vector>

namespace kern {

struct CompilationContext;

class LIRPass {
public:
    virtual ~LIRPass() = default;
    virtual std::string_view name() const = 0;
    virtual void run(LIRModule& module, CompilationContext& ctx) = 0;
};

class LIRPassManager {
public:
    template<typename T, typename... Args>
    void add(Args&&... args) {
        passes_.push_back(std::make_unique<T>(std::forward<Args>(args)...));
    }

    void run(LIRModule& module, CompilationContext& ctx) {
        for (auto& pass : passes_)
            pass->run(module, ctx);
    }

private:
    std::vector<std::unique_ptr<LIRPass>> passes_;
};

} // namespace kern
