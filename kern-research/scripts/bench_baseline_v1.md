# v1 Performance Baseline (M5 complete, tag v0.5-m5)

Date: 2026-03-01
Machine: arm64 macOS (Rosetta 2 for x86-64 binaries)
Tests: 517 unit + 113 E2E

## fib(35) Benchmark

### Compile time (kernc → binary)
| Run | Time |
|-----|------|
| 1   | 57ms |
| 2   | 54ms |
| 3   | 53ms |
| **avg** | **~54ms** |

### Run time (Rosetta 2)
| Run | Time |
|-----|------|
| 1   | 568ms (cold) |
| 2   | 150ms |
| 3   | 149ms |
| **avg (warm)** | **~150ms** |

Result: fib(35) = 9227465 (exit code 201 = 9227465 % 256)

## Notes
- Compile time includes: lex + parse + typecheck + purity + IR + codegen + nasm + ld
- Run time measured via Rosetta 2 (native x86-64 would be faster)
- First run includes Rosetta translation cache warmup
