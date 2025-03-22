#pragma once

#include "00_lib/iterator/Iteratable.hpp"

namespace nugdev::compiler::lib::iterator {

template <typename T> class Context;
template <typename T> struct Iterator : public Iteratable<T, Iterator<T>> {
  public:
    using Iteratable<T, Iterator<T>>::Iteratable;
    using super_t = Iteratable<T, Iterator<T>>;

  public:
    Iterator(Context<T> &context, const super_t::position_t &position) : m_position{position}, m_context{context} {}

  public:
    bool operator==(const Iterator &other) const override { return m_position == other.position(); }
    bool operator!=(const Iterator &other) const override { return m_position != other.position(); }
    bool operator<(const Iterator &other) const override { return m_position < other.position(); }
    bool operator>(const Iterator &other) const override { return m_position > other.position(); }
    bool operator<=(const Iterator &other) const override { return m_position <= other.position(); }
    bool operator>=(const Iterator &other) const override { return m_position >= other.position(); }
    Iterator &operator++() override {
        m_position++;
        return *this;
    }
    Iterator &operator++(int) override {
        m_position++;
        return *this;
    }
    Iterator &operator--() override {
        m_position--;
        return *this;
    }
    Iterator &operator--(int) override {
        m_position--;
        return *this;
    }
    Iterator &operator+=(const super_t::position_t &position) override {
        m_position += position;
        return *this;
    }
    Iterator &operator-=(const super_t::position_t &position) override {
        m_position -= position;
        return *this;
    }
    Iterator operator+(const super_t::position_t &position) const override {
        Iterator result = *this;
        result += position;
        return result;
    }
    Iterator operator-(const super_t::position_t &position) const override {
        Iterator result = *this;
        result -= position;
        return result;
    }
    super_t::position_t operator-(const Iterator &other) const override { return m_position - other.position(); }
    super_t::elem_t operator*() const override { return m_context[m_position]; }

  public:
    bool valid() const override { return m_context.valid(); }
    Iterator &next() override {
        m_position++;
        return *this;
    }
    Iterator &prev() override {
        m_position--;
        return *this;
    }
    Iterator &move(const super_t::position_t &position) override {
        m_position = position;
        return *this;
    }
    super_t::position_t position() const override { return m_position; }
    super_t::elem_t value() const override { return m_context[m_position]; }
    super_t::elem_t value_or(super_t::elem_t &&default_value) const override {
        if (valid() == false) {
            return default_value;
        }
        return value();
    }

  private:
    super_t::position_t m_position;
    Context<T> &m_context;
};
template <typename T> using iterator_t = Iterator<T>;

} // namespace nugdev::compiler::lib::iterator
