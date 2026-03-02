#include "kern/backend/TargetBackend.h"
#include "kern/backend/X86Backend.h"
#include "kern/support/CompilationContext.h"

namespace kern {

TargetBackend* createBackend(TargetArch arch, CompilationContext& ctx) {
    switch (arch) {
        case TargetArch::X86_64:
            return new X86Backend(ctx);
        case TargetArch::AArch64:
            // AArch64 backend not yet implemented
            return nullptr;
    }
    return nullptr;
}

} // namespace kern
