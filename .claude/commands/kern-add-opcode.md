# /kern:add-opcode — Add LIR Opcode (7-File Cascade)

Generate a new LIR opcode and update all affected files in the pipeline.

## Arguments

$ARGUMENTS should be the opcode name (e.g., `CallIndirect`, `FAdd`, `StructAlloc`)

## Cascade Checklist

The agent MUST modify these 7+ files in order:

1. **`include/kern/lir/LIR.h`** — Add `LIROp::<Name>` enum value
2. **`include/kern/lir/LIR.h`** — Add data fields to `LIRInstr` union for the new opcode
3. **`lib/LIR/LIRBuilder.cpp`** — Add HIR → LIR lowering for the new opcode
4. **`lib/LIR/LIRDump.cpp`** — Add text output format (e.g., `"call_indirect %v0(%v1, %v2)"`)
5. **`lib/Backend/InstructionSelector.cpp`** — Add LIR → MachIR instruction selection
6. **`lib/Backend/Emitter.cpp`** — Add MachIR → NASM assembly emission
7. **`tests/unit/lir/LIRTest.cpp`** — Add unit test for the new opcode
8. **`tests/unit/backend/InstSelTest.cpp`** — Add instruction selection test

## Verification

After all modifications:
```bash
/kern:build
```

## Notes

- Follow naming convention: LIROp enum in PascalCase, NASM in lowercase
- Ensure the opcode is reachable from at least one HIR node
- Add an E2E test if the opcode introduces new observable behavior
