#pragma once

#include "00_app/lib/UnicodeString.hpp"
#include "00_app/tag/Tag.h"

namespace nugdev::compiler::generation {

struct ContextTag : public Tag {
    using Tag::Tag;
};
struct Context {
    ContextTag m_id;
    lib::String m_keyword;
    std::optional<lib::String> m_label;
};

} // namespace nugdev::compiler::generation
