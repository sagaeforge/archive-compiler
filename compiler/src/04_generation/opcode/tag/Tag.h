#pragma once

#include "00_app/lib/PointerHelper.hpp"

#include <stduuid/uuid.h>

namespace nugdev::compiler {

// 실제 레지스터를 가르킨다기보단, 가상 레지스터 역할이라고 보면 됨.
class Tag : public lib::PointerHelper<Tag> {
  private:
    Tag(const uuids::uuid &id);

  public:
    Tag() = delete; // 나중에 stl 사용하다가 문제 생기면, 그때 가서 고민.
    Tag(const Tag &);
    Tag(Tag &&) noexcept;
    Tag &operator=(Tag &&) noexcept;
    Tag &operator=(const Tag &) noexcept;

    std::strong_ordering operator<=>(const Tag &) const noexcept;

  public:
    static std::shared_ptr<Tag> create() noexcept;

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
