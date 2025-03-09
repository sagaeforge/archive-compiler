#pragma once

#include "00_app/lib/UnicodeString.hpp"
#include "00_app/tag/Tag.h"

namespace nugdev::compiler::generation {

struct MemoryTag : public Tag {
    using Tag::Tag;
};
class Memory {
  public:
    Memory(const MemoryTag &tag) : m_tag(tag) {}
    Memory(const MemoryTag &tag, const size_t &size) : m_tag(tag), m_data(size, char(0)) {}

  public:
    void allocate(const lib::String &str) {
        auto bufferSize = sizeof(UChar) * str.length();
        if (bufferSize > m_data.size()) {
            throw std::runtime_error("Memory allocation failed");
        }
        std::memcpy(m_data.data(), str.getBuffer(), bufferSize);
    }
    void append(const size_t &size) {
        auto copy = std::vector<char>(size, char(0));
        copy.insert(copy.begin(), m_data.begin(), m_data.end());
        m_data = std::move(copy);
    }
    void append(const Memory &memory) {
        auto copy = std::vector<char>(memory.m_data.size(), char(0));
        copy.insert(copy.begin(), memory.m_data.begin(), memory.m_data.end());
        m_data = std::move(copy);
    }
    template <typename T> void set(const size_t &index, const T &value) {
        if (index + sizeof(T) > m_data.size()) {
            throw std::runtime_error("Memory allocation failed");
        }
        std::memcpy(m_data.data() + index, &value, sizeof(T));
    }

  public:
    char *get_data() { return m_data.data(); }
    char &operator[](const size_t &index) { return m_data[index]; }
    MemoryTag get_tag() const { return m_tag; }

  private:
    MemoryTag m_tag;
    std::vector<char> m_data;
};

} // namespace nugdev::compiler::generation
