#pragma once

#include <vector>

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

        Stream<R>::elem_t operator*() const { return value(); }

        const Stream<R>::elem_t *operator->() const {
            if (!valid()) {
                throw std::runtime_error("Invalid iterator");
            }
            return &stream.m_elems[index];
        }

      public:
        const_iterator next() const { return const_iterator(stream, index + 1); }
        const_iterator prev() const { return const_iterator(stream, index - 1); }
        bool valid() const { return index < stream.m_elems.size(); }
        Stream<R>::distance_t distance() const { return index; }
        Stream<R>::distance_t distance(const const_iterator &other) const { return index - other.index; }

        Stream<R>::elem_t value() const {
            if (!valid()) {
                throw std::runtime_error("Invalid iterator");
            }
            return stream.m_elems[index];
        }
        Stream<R>::elem_t value_or(Stream<R>::elem_t &&default_value) const { return valid() ? *this : default_value; }

      private:
        const Stream<R> &stream;
        size_t index;
    };

  public:
    using elem_t = T;
    using self_t = Stream<elem_t>;
    using distance_t = std::uint32_t;
    using direction_t = int;
    using iterator_t = const_iterator<elem_t>;

  public:
    Stream(const std::vector<elem_t> &_elems) : m_elems(_elems), m_current(const_iterator<elem_t>(*this, 0)) {}
    Stream(const std::initializer_list<elem_t> &_elems) : m_elems(_elems), m_current(const_iterator<elem_t>(*this, 0)) {}
    Stream(const self_t &_other) : m_elems(_other.m_elems), m_current(begin() + (_other.m_current.distance())) {}

  public:
    iterator_t begin() const { return iterator_t(*this, 0); }
    iterator_t end() const { return iterator_t(*this, m_elems.size()); }
    iterator_t current() const { return m_current; }

    bool is_valid(const iterator_t &it) const { return it.valid(); }
    self_t &advance() {
        m_current++;
        return *this;
    }
    self_t clone() const { return self_t(*this); }
    self_t move(const direction_t &dir) {
        m_current += dir;
        return *this;
    }
    self_t move_at(const iterator_t &it) {
        m_current = it;
        return *this;
    }
    self_t move_at(const distance_t &dist) {
        m_current = dist;
        return *this;
    }
    self_t next() {
        m_current++;
        return *this;
    }
    self_t prev() {
        m_current--;
        return *this;
    }

  private:
    std::vector<elem_t> m_elems;
    iterator_t m_current;
};

using StringStream = Stream<char16_t>;
using StringStreamIterator = StringStream::iterator_t;

template <typename T> Stream<T> make_stream(const std::vector<T> &elems) { return Stream<T>(elems); }
template <typename T> Stream<T> make_stream(const std::initializer_list<T> &elems) { return Stream<T>(elems); }
StringStream make_stream(const icu::UnicodeString &str);

} // namespace nugdev::compiler::stream