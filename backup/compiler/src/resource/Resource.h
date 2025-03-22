#pragma once

#include "00_lib/lib/PointerHelper.hpp"
#include "00_lib/lib/String.h"
#include "00_lib/lib/Tag.h"

namespace nugdev::compiler::lib {

class ResourceStream;

class ResourceTag : public Tag {
    using Tag::Tag;
};
struct ResourcePosition {
    ResourceTag tag;
    std::uint32_t row;
    std::uint32_t column;
};
class Resource : public PointerHelper<Resource> {
  public:
    // 전역
    virtual ResourceTag get_tag() const = 0;
    virtual std::uint32_t get_size() const = 0;
    virtual Char &at(std::uint32_t index) const = 0;

    // 스트림
    virtual ResourceStream stream() const = 0;

    // 컨텐츠 복구를 위한 함수.
    virtual void set_content(const ResourcePosition &position, const String &content) = 0;
    virtual bool has_overwrite(const ResourcePosition &position, const String &content) const = 0;
};
using ResourcePtr = std::shared_ptr<Resource>;

} // namespace nugdev::compiler::lib

#include "00_lib/resource/ResourceStream.hpp"