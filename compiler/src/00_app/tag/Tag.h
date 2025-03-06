#pragma once

#include "00_app/lib/PointerHelper.hpp"

#include <memory>
#include <stduuid/uuid.h>

namespace nugdev::compiler {

class Tag : public lib::PointerHelper<Tag> {
  private:
    Tag();
    Tag(const uuids::uuid &id);

  public:
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
