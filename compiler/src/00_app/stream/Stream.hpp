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

        Stream<R>::elem_t operator*() const { return stream.str[index]; }

      public:
        const_iterator next() const { return const_iterator(stream, index + 1); }
        const_iterator prev() const { return const_iterator(stream, index - 1); }
        bool vaild() const { return index < stream.str.length(); }

        Stream<R>::elem_t value_or(Stream<R>::elem_t &&default_value) const { return vaild() ? *this : default_value; }

      private:
        const Stream<R> &stream;
        size_t index;
    };

  public:
    using elem_t = T;

  public:
    Stream(icu::UnicodeString str) : str(str), current(const_iterator<elem_t>(*this, 0)) {}

  public:
    const_iterator<elem_t> begin() const { return const_iterator<elem_t>(*this, 0); }
    const_iterator<elem_t> end() const { return const_iterator<elem_t>(*this, str.length()); }

    const_iterator<elem_t> checkpoint() const { return const_iterator<elem_t>(current); }
    void rollback() {
        current = checkpoints.back();
        checkpoints.pop_back();
    }
    void commit(const const_iterator<elem_t> &checkpoint) {
        checkpoints.push_back(checkpoint);
        current = checkpoint;
    }
    bool is_vaild(const const_iterator<elem_t> &it) const { return it.vaild(); }
    const_iterator<elem_t> find_first_of(const std::vector<elem_t> &chars) {
        for (auto it = current; it != end(); ++it) {
            if (std::ranges::find(chars, *it) != chars.end()) {
                return it;
            }
        }
        return end();
    }
    void advance() { current++; }

  private:
    icu::UnicodeString str;
    std::deque<const_iterator<elem_t>> checkpoints;
    const_iterator<elem_t> current;
};

using StringStream = Stream<char16_t>;
using StringStreamIterator = StringStream::const_iterator<StringStream::elem_t>;

} // namespace nugdev::compiler::stream
