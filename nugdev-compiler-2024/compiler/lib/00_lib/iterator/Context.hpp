#pragma once

#include "00_lib/iterator/Iterator.hpp"
#include "00_lib/lib/Exception.h"

namespace nugdev::compiler::lib::iterator {

struct ContextInvalidException : public Exception {
    DEFINE_DEFAULT_EXCEPTION_CONSTRUCTOR_WITH_MESSAGE(ContextInvalidException, Exception, "Context is invalid")
};

template <typename T>
class Context {
public:
    using self_t = Context;
    using iterator_t = iterator_t<T>;
    using elem_t = T;

public:
    iterator_t &current() {
        return m_current;
    }

public:
    Context(const Context &other) : m_values{other.m_values}, m_current{*this, other.m_current.position()} {
    }
    Context(Context &&other) noexcept : m_values{std::move(other.m_values)}, m_current{*this, other.m_current.position()} {
    }
    Context(const std::vector<T> &values) : m_values{values}, m_current{*this, 0} {
    }

public:
    elem_t operator[](const iterator_t::position_t &position) const {
        return m_values[position];
    }

public:
    bool valid() const {
        if (m_current.position() >= m_values.size()) {
            return false;
        }
        return true;
    }

private:
    iterator_t m_current;
    std::vector<T> m_values;
};
template <typename T>
using context_t = Context<T>;

template <typename T>
class ContextCommand {
public:
    using self_t = ContextCommand;
    using iterator_t = iterator_t<T>;

public:
    ContextCommand(Context<T> &context) : m_context{context} {
    }

public:
    self_t next() const {
        m_context.current().next();
        return *this;
    }
    self_t prev() const {
        m_context.current().prev();
        return *this;
    }
    self_t move(const iterator_t::position_t &position) const {
        m_context.current().move(position);
        return *this;
    }
    iterator_t::position_t position() const {
        return m_context.current().position();
    }
    iterator_t::elem_t value() const {
        return m_context.current().value();
    }
    iterator_t current() const {
        return m_context.current();
    }
    bool valid() const {
        return m_context.valid();
    }

private:
    Context<T> &m_context;
};
template <typename T>
using command_t = ContextCommand<T>;

}  // namespace nugdev::compiler::lib::iterator
