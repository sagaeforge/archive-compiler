#pragma once

#include "04_generation/opcode/tag/Tag.h"

namespace nugdev::compiler::generation {

// 실제 레지스터를 가르킨다기보단, 가상 레지스터 역할이라고 보면 됨.
class RegisterTag : public compiler::Tag {};

} // namespace nugdev::compiler::generation

namespace std {
template <> struct hash<nugdev::compiler::generation::RegisterTag> {
    std::size_t operator()(const nugdev::compiler::generation::RegisterTag &tag) const noexcept { return tag.hash(); }
};
} // namespace std
