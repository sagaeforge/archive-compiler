#pragma once

#include <stduuid/uuid.h>

namespace nugdev::compiler {

// 실제 레지스터를 가르킨다기보단, 가상 레지스터 역할이라고 보면 됨.
class Tag {
  private:
    Tag(const uuids::uuid &id);

  public:
    Tag() = delete;
    Tag(const Tag &);
    Tag(Tag &&) noexcept;
    Tag &operator=(Tag &&) noexcept;
    Tag &operator=(const Tag &) noexcept;

    std::strong_ordering operator<=>(const Tag &) const noexcept;

  public:
    static Tag create() noexcept;

  public:
    std::size_t hash() const;

  private:
    uuids::uuid m_id;
    std::size_t m_hash;
};

} // namespace nugdev::compiler

namespace std {
template <> struct hash<nugdev::compiler::Tag> {
    std::size_t operator()(const nugdev::compiler::Tag &tag) const noexcept { return tag.hash(); }
};

} // namespace std
