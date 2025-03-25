#pragma once

#include <source_location>
#include <type_traits>

#include "00_lib/lib/String.h"

namespace nugdev::compiler::lib {

class Exception : public std::exception {
public:
    Exception(const String &message, const std::source_location &location);
    const char *what() const noexcept override;

private:
    String m_message;
    std::string m_message_str;
    std::source_location m_location;
};

#define DEFINE_DEFAULT_EXCEPTION_CONSTRUCTOR(CLASS, SUPER)                                                                                                                                             \
public:                                                                                                                                                                                                \
    CLASS(const nugdev::compiler::lib::String &message, const std::source_location &location) : SUPER(message, location) {                                                                             \
    }

#define DEFINE_DEFAULT_EXCEPTION_CONSTRUCTOR_WITH_MESSAGE(CLASS, SUPER, MESSAGE)                                                                                                                       \
public:                                                                                                                                                                                                \
    CLASS(const std::source_location &location) : SUPER(MESSAGE, location) {                                                                                                                           \
    }                                                                                                                                                                                                  \
    DEFINE_DEFAULT_EXCEPTION_CONSTRUCTOR(CLASS, SUPER)

template <typename ExceptionType>
    requires std::is_base_of_v<Exception, ExceptionType>
ExceptionType throw_exception(const std::source_location &loc = std::source_location::current()) {
    throw ExceptionType(loc);
}

template <typename ExceptionType>
    requires std::is_base_of_v<Exception, ExceptionType>
ExceptionType throw_exception(const String &message, const std::source_location &loc = std::source_location::current()) {
    throw ExceptionType(message, loc);
}

class OutOfRangeException : public Exception {
public:
    DEFINE_DEFAULT_EXCEPTION_CONSTRUCTOR(OutOfRangeException, Exception)
};

class Nothing : public Exception {
public:
    DEFINE_DEFAULT_EXCEPTION_CONSTRUCTOR_WITH_MESSAGE(Nothing, Exception, "Nothing")
};
#define TODO() throw_exception<Nothing>(std::source_location::current())

struct Requires {
    using self_t = Requires;

    bool m_condition;

    template <typename ExceptionType, typename... Args>
    self_t throws(Args &&...args) {
        if (m_condition == false) {
            throw_exception<ExceptionType>(std::forward<Args>(args)...);
        }
        return *this;
    }
};

struct InfiniteLoopDetectedException : public Exception {
    DEFINE_DEFAULT_EXCEPTION_CONSTRUCTOR_WITH_MESSAGE(InfiniteLoopDetectedException, Exception, "Infinite loop detected")
};

}  // namespace nugdev::compiler::lib