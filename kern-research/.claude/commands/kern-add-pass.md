# /kern:add-pass — Add HIR/LIR Pass Boilerplate

Generate boilerplate files for a new compiler pass.

## Arguments

$ARGUMENTS should be: `<PassName> --level hir|lir`

Example: `/kern:add-pass PurityAnalysis --level hir`

## Generated Files

For `--level hir`:
1. `include/kern/hir/<PassName>Pass.h` — Pass class declaration
2. `lib/HIR/<PassName>Pass.cpp` — Pass implementation
3. `tests/unit/hir/<PassName>PassTest.cpp` — Unit tests

For `--level lir`:
1. `include/kern/lir/<PassName>Pass.h` — Pass class declaration
2. `lib/LIR/<PassName>Pass.cpp` — Pass implementation
3. `tests/unit/lir/<PassName>PassTest.cpp` — Unit tests

## Template

```cpp
// include/kern/<level>/<PassName>Pass.h
#pragma once
#include "kern/<level>/<Level>.h"

namespace kern {

class <PassName>Pass {
public:
    void run(<Level>Module& module);
};

} // namespace kern
```

## Post-Generation Steps

1. Add the .cpp file to the appropriate `lib/<Level>/CMakeLists.txt`
2. Add the test file to `tests/unit/CMakeLists.txt`
3. Register the pass in the PassManager if applicable
4. Run `/kern:build` to verify compilation
