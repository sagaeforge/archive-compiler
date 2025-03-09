#pragma once

#include "00_app/lib/PointerHelper.hpp"
#include "00_app/lib/UnicodeString.hpp"

#include <stduuid/uuid.h>

namespace nugdev::compiler {

class Tag : public lib::PointerHelper<Tag> {
  private:
    Tag();
    Tag(const uuids::uuid &id);

  public:
    Tag(const lib::String &str);
    Tag(const Tag &);
    Tag(Tag &&) noexcept;
    Tag &operator=(Tag &&) noexcept;
    Tag &operator=(const Tag &) noexcept;
    Tag &operator=(const lib::String &str);

    bool operator==(const Tag &) const noexcept;
    std::strong_ordering operator<=>(const Tag &) const noexcept;

  public:
    template <typename T>
        requires std::is_base_of<Tag, T>::value
    static T create() noexcept {
        static std::mt19937 engine(std::random_device{}());
        static uuids::uuid_random_generator generator(engine);
        return T(generator());
    }
    template <typename T>
        requires std::is_base_of<Tag, T>::value
    static T empty() noexcept {
        return T();
    }

  public:
    std::size_t hash() const;
    lib::String to_str() const;

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
