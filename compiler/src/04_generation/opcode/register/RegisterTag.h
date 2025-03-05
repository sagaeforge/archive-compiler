#pragma once

#include <stduuid/uuid.h>

namespace nugdev::compiler::generation {

class RegisterTag {
  private:
    RegisterTag(const uuids::uuid &id);

  public:
    RegisterTag() = delete;
    RegisterTag(const RegisterTag &);
    RegisterTag(RegisterTag &&) noexcept;
    RegisterTag &operator=(RegisterTag &&) noexcept;
    RegisterTag &operator=(const RegisterTag &) noexcept;

    std::strong_ordering operator<=>(const RegisterTag &) const noexcept;

  public:
    static RegisterTag create() noexcept;

  public:
    std::size_t hash() const;

  private:
    uuids::uuid m_id;
    std::size_t m_hash;
};

} // namespace nugdev::compiler::generation

namespace std {
template <> struct hash<nugdev::compiler::generation::RegisterTag> {
    std::size_t operator()(const nugdev::compiler::generation::RegisterTag &tag) const noexcept { return tag.hash(); }
};
} // namespace std
