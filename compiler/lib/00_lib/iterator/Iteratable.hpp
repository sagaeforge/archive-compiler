#pragma once

#include <cstdint>

namespace nugdev::compiler::lib::iterator {

template <typename T, typename Child>
class Iteratable {
public:
    using self_t = Iteratable<T, Child>;
    using elem_t = T;
    using child_t = Child;
    using position_t = std::uint32_t;

public:
    virtual bool operator==(const child_t &other) const = 0;
    virtual bool operator!=(const child_t &other) const = 0;
    virtual bool operator<(const child_t &other) const = 0;
    virtual bool operator>(const child_t &other) const = 0;
    virtual bool operator<=(const child_t &other) const = 0;
    virtual bool operator>=(const child_t &other) const = 0;
    virtual child_t &operator++() = 0;
    virtual child_t &operator++(int) = 0;
    virtual child_t &operator--() = 0;
    virtual child_t &operator--(int) = 0;
    virtual child_t &operator+=(const position_t &position) = 0;
    virtual child_t &operator-=(const position_t &position) = 0;
    virtual child_t operator+(const position_t &position) const = 0;
    virtual child_t operator-(const position_t &position) const = 0;
    virtual position_t operator-(const child_t &other) const = 0;
    virtual elem_t operator*() const = 0;

public:
    virtual bool valid() const = 0;
    virtual child_t &next() = 0;
    virtual child_t &prev() = 0;
    virtual child_t &move(const position_t &position) = 0;
    virtual position_t position() const = 0;
    virtual elem_t value() const = 0;
    virtual elem_t value_or(elem_t &&default_value) const = 0;
};
template <typename T, typename Child>
using iteratable_t = Iteratable<T, Child>;

}  // namespace nugdev::compiler::lib::iterator
