#pragma once
#include "kern/debug/DebugInfo.h"
#include "kern/support/TypeSystem.h"
#include <cstdint>
#include <string>
#include <functional>

namespace kern {

// Interprets raw bytes from a target process's memory
// according to Kern type information.
// Used by kern-dbg to display variable values.

class ValueInspector {
public:
    explicit ValueInspector(const TypeTable& types);

    // Format a value of the given type from raw bytes.
    // `bytes` points to `size` bytes read from the target process.
    std::string format(TypeId type, const uint8_t* bytes, size_t size) const;

    // Return the expected byte size for a type.
    size_t sizeOf(TypeId type) const;

    // Format a local variable given its debug info and a memory reader.
    // `read_memory` reads `size` bytes from address `addr` into `buf`.
    using MemoryReader = std::function<bool(uint64_t addr, void* buf, size_t size)>;
    std::string formatLocal(const LocalVarInfo& var, uint64_t rbp,
                            const MemoryReader& read_memory) const;

private:
    const TypeTable& types_;

    std::string formatPrimitive(const TypeInfo& info, const uint8_t* bytes,
                                size_t size) const;
    std::string formatPointer(const TypeInfo& info, const uint8_t* bytes) const;
    std::string formatStruct(const TypeInfo& info, const uint8_t* bytes,
                             size_t size) const;
};

} // namespace kern
