#include "kern/debug/DebugInfoBuilder.h"
#include "kern/backend/MachIR.h"
#include <cstring>

namespace kern {

DebugInfoBuilder::DebugInfoBuilder(CompilationContext& ctx) : ctx_(ctx) {}

DebugInfo DebugInfoBuilder::build(const MachModule& mod) {
    DebugInfo info;
    uint64_t offset = 0;

    for (uint32_t i = 0; i < mod.fn_count; ++i) {
        const auto& fn = mod.functions[i];
        if (fn.is_intrinsic) continue;

        auto fdi = buildFunction(fn);
        fdi.code_start = offset;

        // Estimate code size: sum of instructions * avg instruction size
        uint32_t instr_count = 0;
        for (uint32_t b = 0; b < fn.block_count; ++b) {
            instr_count += fn.blocks[b].instr_count;
        }
        // Rough estimate: 5 bytes per instruction average
        offset += static_cast<uint64_t>(instr_count) * 5;
        fdi.code_end = offset;

        info.functions.push_back(std::move(fdi));
    }

    return info;
}

FunctionDebugInfo DebugInfoBuilder::buildFunction(const MachFunction& fn) {
    FunctionDebugInfo fdi;
    fdi.name = fn.name;
    fdi.source_loc = SourceLocation::unknown();

    // Extract local variables from stack slots.
    // In our current MachIR, locals are stack operands [rbp-offset].
    // We scan instructions for stack operands to identify local slots.
    // NOTE: Full local variable tracking requires HIR→MachIR metadata
    // propagation, which will be added in Phase 5 integration.
    // For now, we record stack slots as anonymous locals.

    // Track unique stack offsets
    struct StackSlot {
        int32_t offset;
        uint8_t width;
    };
    std::vector<StackSlot> slots;

    for (uint32_t b = 0; b < fn.block_count; ++b) {
        const auto& blk = fn.blocks[b];
        for (uint32_t j = 0; j < blk.instr_count; ++j) {
            const auto& instr = blk.instrs[j];
            for (uint8_t k = 0; k < instr.operand_count; ++k) {
                const auto& op = instr.operand(k);
                if (op.isStack()) {
                    bool found = false;
                    for (const auto& s : slots) {
                        if (s.offset == op.stack_offset) {
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        slots.push_back({op.stack_offset, instr.width});
                    }
                }
            }
        }
    }

    // Create anonymous local vars from stack slots
    uint32_t idx = 0;
    for (const auto& slot : slots) {
        LocalVarInfo var;
        // Name will be interned placeholder until HIR propagation
        var.name = ctx_.strings.intern(
            std::string("_slot") + std::to_string(idx++));
        var.stack_offset = slot.offset;
        var.type = 0;  // Unknown until HIR metadata propagation
        fdi.locals.push_back(var);
    }

    return fdi;
}

// Binary serialization format:
// [uint32_t fn_count]
// For each function:
//   [uint32_t name_len] [chars...] [uint64_t code_start] [uint64_t code_end]
//   [uint32_t local_count]
//   For each local:
//     [uint32_t name_len] [chars...] [uint32_t type] [int32_t offset]
//     [uint32_t scope_start] [uint32_t scope_end]
// [uint32_t mapping_count]
// For each mapping:
//   [uint64_t addr] [uint32_t line] [uint32_t col]
//   [uint32_t file_len] [chars...]

static void writeU32(std::ostream& out, uint32_t v) {
    out.write(reinterpret_cast<const char*>(&v), 4);
}

static void writeU64(std::ostream& out, uint64_t v) {
    out.write(reinterpret_cast<const char*>(&v), 8);
}

static void writeI32(std::ostream& out, int32_t v) {
    out.write(reinterpret_cast<const char*>(&v), 4);
}

static void writeStr(std::ostream& out, std::string_view s) {
    writeU32(out, static_cast<uint32_t>(s.size()));
    out.write(s.data(), static_cast<std::streamsize>(s.size()));
}

static uint32_t readU32(std::istream& in) {
    uint32_t v = 0;
    in.read(reinterpret_cast<char*>(&v), 4);
    return v;
}

static uint64_t readU64(std::istream& in) {
    uint64_t v = 0;
    in.read(reinterpret_cast<char*>(&v), 8);
    return v;
}

static int32_t readI32(std::istream& in) {
    int32_t v = 0;
    in.read(reinterpret_cast<char*>(&v), 4);
    return v;
}

static std::string readStr(std::istream& in) {
    uint32_t len = readU32(in);
    std::string s(len, '\0');
    in.read(s.data(), len);
    return s;
}

void DebugInfoBuilder::serialize(const DebugInfo& info, std::ostream& out) {
    // Magic header
    out.write("KDBI", 4);

    // Functions
    writeU32(out, static_cast<uint32_t>(info.functions.size()));
    for (const auto& fn : info.functions) {
        writeStr(out, fn.name);
        writeU64(out, fn.code_start);
        writeU64(out, fn.code_end);
        writeU32(out, fn.source_loc.line);
        writeU32(out, fn.source_loc.col);
        writeStr(out, fn.source_loc.filename);

        writeU32(out, static_cast<uint32_t>(fn.locals.size()));
        for (const auto& var : fn.locals) {
            writeStr(out, var.name);
            writeU32(out, var.type);
            writeI32(out, var.stack_offset);
            writeU32(out, var.scope_start_line);
            writeU32(out, var.scope_end_line);
        }
    }

    // Source mappings
    writeU32(out, static_cast<uint32_t>(info.source_map.size()));
    for (const auto& m : info.source_map) {
        writeU64(out, m.addr);
        writeU32(out, m.line);
        writeU32(out, m.column);
        writeStr(out, m.file);
    }
}

DebugInfo DebugInfoBuilder::deserialize(std::istream& in) {
    DebugInfo info;

    // Verify magic
    char magic[4];
    in.read(magic, 4);
    if (std::memcmp(magic, "KDBI", 4) != 0) {
        return info;  // empty on bad magic
    }

    // Functions
    uint32_t fn_count = readU32(in);
    info.functions.reserve(fn_count);
    for (uint32_t i = 0; i < fn_count; ++i) {
        FunctionDebugInfo fn;
        auto name_str = readStr(in);
        fn.name = std::string_view(name_str);  // Note: lifetime managed externally
        fn.code_start = readU64(in);
        fn.code_end = readU64(in);
        fn.source_loc.line = readU32(in);
        fn.source_loc.col = readU32(in);
        auto file_str = readStr(in);
        fn.source_loc.filename = std::string_view(file_str);

        uint32_t local_count = readU32(in);
        fn.locals.reserve(local_count);
        for (uint32_t j = 0; j < local_count; ++j) {
            LocalVarInfo var;
            auto var_name = readStr(in);
            var.name = std::string_view(var_name);
            var.type = readU32(in);
            var.stack_offset = readI32(in);
            var.scope_start_line = readU32(in);
            var.scope_end_line = readU32(in);
            fn.locals.push_back(var);
        }

        info.functions.push_back(std::move(fn));
    }

    // Source mappings
    uint32_t map_count = readU32(in);
    info.source_map.reserve(map_count);
    for (uint32_t i = 0; i < map_count; ++i) {
        SourceMapping m;
        m.addr = readU64(in);
        m.line = readU32(in);
        m.column = readU32(in);
        auto file = readStr(in);
        m.file = std::string_view(file);
        info.source_map.push_back(m);
    }

    return info;
}

} // namespace kern
