# /kern:add-tool — Scaffold New Tool Binary

Generate the directory structure and CMake configuration for a new tool.

## Arguments

$ARGUMENTS should be the tool name (e.g., `kern-lsp`, `kern-dbg`, `kern-fmt`)

## Generated Files

1. `tools/$ARGUMENTS/main.cpp` — Entry point
2. `tools/$ARGUMENTS/CMakeLists.txt` — Build configuration with library dependencies

## Template

```cpp
// tools/<name>/main.cpp
#include <cstdio>
#include <cstdlib>

int main(int argc, char* argv[]) {
    // TODO: Implement <name>
    std::fprintf(stderr, "<name>: not yet implemented\n");
    return EXIT_FAILURE;
}
```

```cmake
# tools/<name>/CMakeLists.txt
kern_add_tool(<name>
    main.cpp
    DEPENDS kern_support  # Add more dependencies as needed
)
```

## Post-Generation Steps

1. Add `add_subdirectory(tools/$ARGUMENTS)` to root `CMakeLists.txt`
2. Update the library dependency list based on the tool's requirements (see design doc Section 4 matrix)
3. Run `/kern:build`
4. Create `tests/tool/$ARGUMENTS/` directory for tool-specific tests

## Notes

- Refer to the dependency matrix in the architecture doc for correct library links
- Tools should be thin wrappers — all logic goes in libraries
