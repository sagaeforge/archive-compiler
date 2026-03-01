# Layer Boundaries

Each library lives in `lib/<Name>/` with headers in `include/kern/<name>/`.
Dependencies flow strictly downward. Reverse dependencies are blocked by the layer-guard hook.

## Dependency Matrix

| Layer | May depend on |
|-------|--------------|
| **Support** | (nothing — base layer) |
| **Lexer** | Support |
| **Parser** | Support, Lexer |
| **HIR** | Support, Lexer, Parser |
| **LIR** | Support, HIR |
| **Backend** | Support, LIR |
| **IDE** | Support, Lexer, Parser, HIR |
| **Debug** | Support |
| **Fmt** | Support, Lexer, Parser |
| **Lint** | Support, Lexer, Parser, HIR |
| **Pipeline** | All libs (orchestrator) |

## File Scope Rules

When editing files in a layer, you may only `#include` headers from allowed dependencies:

```
// In lib/LIR/LIRBuilder.cpp — OK:
#include "kern/lir/LIR.h"        // own layer
#include "kern/hir/HIR.h"        // HIR (allowed)
#include "kern/support/Arena.h"  // Support (allowed)

// BLOCKED:
#include "kern/backend/Emitter.h"  // Backend (reverse dependency!)
```

## Adding New Dependencies

If a new dependency is needed that violates the matrix:
1. Ask the user first — this is an architectural decision
2. Consider if the dependency should be in Support instead
3. Never add a reverse dependency as a shortcut
