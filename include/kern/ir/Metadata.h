#pragma once
#include <cstdint>

namespace kern {

enum class Purity : uint8_t {
    Pure,
    ImpureMut,
    ImpureIo,
    ImpureMem,
    Unknown
};

inline const char* purityName(Purity p) {
    switch (p) {
        case Purity::Pure:      return "pure";
        case Purity::ImpureMut: return "impure(mut)";
        case Purity::ImpureIo:  return "impure(io)";
        case Purity::ImpureMem: return "impure(mem)";
        case Purity::Unknown:   return "unknown";
    }
    return "?";
}

struct FunctionMeta {
    Purity purity = Purity::Unknown;
    bool is_recursive = false;
    bool is_tailrec = false;
};

} // namespace kern
