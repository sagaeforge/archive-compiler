# Architecture Invariants

- string_view lifetime: tokens/AST hold string_views into source. Source must outlive all consumers.
- IR: SSA with block arguments, no phi nodes.
  Merge block params: [result_val, then_val, else_val, then_block, else_block]
- ABI: System V AMD64 (rdi, rsi, rdx, rcx, r8, r9 → rax return)
- Arena allocator: bump alloc in 4096-byte blocks for all AST/IR nodes. Never raw new.
- TypeChecker memoizes Expr*→Type in expr_types_ (Arena guarantees pointer stability)
- PurityChecker: Kahn's algorithm (reverse topo sort) + DFS (recursion detection)
- macOS linking: -platform_version macos 14.0.0 14.0.0 -arch x86_64 -lSystem
