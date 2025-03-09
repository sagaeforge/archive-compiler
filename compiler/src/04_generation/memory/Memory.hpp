#pragma once

#include "00_app/lib/UnicodeString.hpp"
#include "00_app/tag/Tag.h"
#include <cstring>
#include <memory>
#include <stdexcept>

namespace nugdev::compiler::generation {

struct MemoryTag : public Tag {
    using Tag::Tag;
};

class Memory {
  public:
    class Reference {
      public:
        Reference(Memory &memory, size_t offset, size_t alignment) : m_memory(memory), m_offset(offset), m_alignment(alignment) {}

      public:
        template <typename T> Reference &operator=(const T &value) {
            if (m_alignment == 1 && m_offset == 0) {
                // 찾기 어려운 버그가 발생할 것 같긴한데, 일단 이렇게 해둠.
                m_memory.align(value);
                return *this;
            }

            if (sizeof(T) > m_alignment) {
                throw std::runtime_error("Reference can only be assigned to types with alignment <= m_alignment");
            }

            m_memory.set(m_offset, value);
            return *this;
        }

      private:
        Memory &m_memory;
        size_t m_offset;
        size_t m_alignment;
    };

  public:
    Memory(const MemoryTag &tag) : m_tag(tag), m_data(nullptr), m_size(0), m_alignment(1), m_capacity(0) {}
    Memory(const MemoryTag &tag, const size_t &length, const std::uint32_t &alignment = 1)
        : m_tag(tag), m_data(new char[length * alignment]), m_size(alignment * length), m_alignment(alignment), m_capacity(alignment * length) {}
    Memory(const MemoryTag &tag, const lib::String &str)
        : m_tag(tag), m_data(new char[sizeof(UChar) * str.length()]), m_size(sizeof(UChar) * str.length()), m_alignment(sizeof(UChar)),
          m_capacity(sizeof(UChar) * str.length()) {
        // 문자열을 위한 메모리 할당
        if (m_size > 0) {
            std::memcpy(m_data.get(), str.getBuffer(), m_size);
        }
    }

  public:
    void reserve(const size_t &size) {
        if (size > m_capacity) {
            auto newData = std::shared_ptr<char[]>(new char[size]);
            std::memcpy(newData.get(), m_data.get(), m_size);
            m_data = newData;
            m_capacity = size;
        }
    }
    void resize(const size_t &size) {
        if (size % m_alignment != 0) {
            throw std::runtime_error("Size must be a multiple of alignment");
        }

        auto newData = std::shared_ptr<char[]>(new char[size]);
        std::memset(newData.get(), 0, size);
        m_data = newData;
        m_size = size;
        m_capacity = size;
    }
    void append(const Memory &memory) {
        // 만약에 케파가 부족하면 메모리를 증가시킴.
        if (m_size + memory.m_size > m_capacity) {
            reserve(m_size + memory.m_size);
        }

        std::memcpy(m_data.get() + m_size, memory.m_data.get(), memory.m_size);
        m_size += memory.m_size;
    }
    template <typename T> void append(const T &value) {
        if (m_size + sizeof(T) > m_capacity) {
            reserve(m_size + sizeof(T));
        }

        std::memcpy(m_data.get() + m_size, &value, sizeof(T));
        m_size += sizeof(T);
    }
    template <typename T> void align(const T &value) {
        auto alignment = sizeof(T);
        if (m_size < alignment) {
            reserve(alignment);
        }
        m_alignment = alignment;
        std::memcpy(m_data.get() + m_size, &value, alignment);
        m_size += alignment;
    }
    template <typename T> void set(const size_t &index, const T &value) {
        auto alignment = sizeof(T);
        if (index + alignment > m_size) {
            throw std::runtime_error("Memory allocation failed");
        }
        m_alignment = alignment;
        std::memcpy(m_data.get() + index, &value, alignment);
    }

  public:
    Reference operator[](const std::uint32_t &offset) { return Reference(*this, offset, m_alignment); }

  public:
    MemoryTag get_tag() const { return m_tag; }
    std::uint32_t get_size() const { return m_size; }
    std::uint32_t get_alignment() const { return m_alignment; }

  private:
    MemoryTag m_tag;
    std::shared_ptr<char[]> m_data;
    std::uint32_t m_size;
    std::uint32_t m_alignment;
    std::uint32_t m_capacity;
};

} // namespace nugdev::compiler::generation
