#pragma once

#include <deque>
#include <unicode/unistr.h>

namespace nugdev::compiler::stream {

template <typename T = char16_t> class Stream {
  public:
    template <typename R> class const_iterator {
      public:
        const_iterator(const Stream &stream, size_t index) : stream(stream), index(index) {}
        const_iterator(const const_iterator &other) : stream(other.stream), index(other.index) {}

        const_iterator &operator=(const const_iterator &other) {
            if (this != &other) {
                index = other.index;
            }
            return *this;
        }

      public:
        bool operator==(const const_iterator &other) const { return index == other.index; }
        bool operator!=(const const_iterator &other) const { return index != other.index; }
        bool operator<(const const_iterator &other) const { return index < other.index; }
        bool operator>(const const_iterator &other) const { return index > other.index; }
        bool operator<=(const const_iterator &other) const { return index <= other.index; }
        bool operator>=(const const_iterator &other) const { return index >= other.index; }

        const_iterator &operator++() {
            index++;
            return *this;
        }
        const_iterator operator++(int) {
            const_iterator tmp = *this;
            index++;
            return tmp;
        }
        const_iterator &operator--() {
            index--;
            return *this;
        }
        const_iterator operator--(int) {
            const_iterator tmp = *this;
            index--;
            return tmp;
        }

        const_iterator &operator+=(size_t n) {
            index += n;
            return *this;
        }
        const_iterator &operator-=(size_t n) {
            index -= n;
            return *this;
        }

        const_iterator operator+(size_t n) const { return const_iterator(stream, index + n); }
        const_iterator operator-(size_t n) const { return const_iterator(stream, index - n); }
        std::uint32_t operator-(const const_iterator &other) const { return distance(other); }

        Stream<R>::elem_t operator*() const { return stream.m_elems[index]; }

      public:
        const_iterator next() const { return const_iterator(stream, index + 1); }
        const_iterator prev() const { return const_iterator(stream, index - 1); }
        bool valid() const { return index < stream.m_elems.size(); }
        std::uint32_t distance() const { return index; }
        std::uint32_t distance(const const_iterator &other) const { return index - other.index; }

        Stream<R>::elem_t value() const { return stream.m_elems[index]; }
        Stream<R>::elem_t value_or(Stream<R>::elem_t &&default_value) const { return valid() ? *this : default_value; }

      private:
        const Stream<R> &stream;
        size_t index;
    };

  public:
    using elem_t = T;
    using self_t = Stream<elem_t>;

  public:
    Stream(const std::vector<elem_t> &_elems) : m_elems(_elems), m_current(const_iterator<elem_t>(*this, 0)) {}
    Stream(const std::initializer_list<elem_t> &_elems) : m_elems(_elems), m_current(const_iterator<elem_t>(*this, 0)) {}
    Stream(const self_t &_other) : m_elems(_other.m_elems), m_current(begin() + (_other.m_current.distance())) {}

  public:
    const_iterator<elem_t> begin() const { return const_iterator<elem_t>(*this, 0); }
    const_iterator<elem_t> end() const { return const_iterator<elem_t>(*this, m_elems.size()); }
    const_iterator<elem_t> current() const { return m_current; }

    bool is_valid(const const_iterator<elem_t> &it) const { return it.valid(); }
    self_t &advance() {
        m_current++;
        return *this;
    }
    self_t clone() const { return self_t(m_elems); }
    self_t move(const const_iterator<elem_t> &it) {
        m_current = it;
        return *this;
    }
    self_t move(int dir) {
        m_current += dir;
        return *this;
    }

  private:
    std::vector<elem_t> m_elems;
    const_iterator<elem_t> m_current;
};

using StringStream = Stream<char16_t>;
using StringStreamIterator = StringStream::const_iterator<StringStream::elem_t>;

template <typename T> Stream<T> make_stream(const std::vector<T> &elems) { return Stream<T>(elems); }
template <typename T> Stream<T> make_stream(const std::initializer_list<T> &elems) { return Stream<T>(elems); }
StringStream make_stream(const icu::UnicodeString &str);

} // namespace nugdev::compiler::stream