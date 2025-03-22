#pragma once

#include "00_lib/lib/String.h"

#include <stduuid/uuid.h>

namespace nugdev::compiler::lib {

class Tag {
  public:
    Tag();
    Tag(const lib::String &str);
    Tag(const Tag &);
    Tag(Tag &&) noexcept;

  public:
    Tag &operator=(Tag &&) noexcept;
    Tag &operator=(const Tag &) noexcept;
    Tag &operator=(const lib::String &str);
    bool operator==(const Tag &) const noexcept;
    std::strong_ordering operator<=>(const Tag &) const noexcept;

  public:
    std::size_t hash() const;
    lib::String to_str() const;

  private:
    uuids::uuid m_id;
    std::size_t m_hash;
};

template <typename Tag, typename... Args>
    requires std::is_base_of_v<Tag, Tag>
Tag make_tag(Args &&...args) {
    return Tag(std::forward<Args>(args)...);
}

} // namespace nugdev::compiler::lib

namespace std {
template <> struct hash<nugdev::compiler::lib::Tag> {
    std::size_t operator()(const nugdev::compiler::lib::Tag &tag) const noexcept { return tag.hash(); }
};

} // namespace std
