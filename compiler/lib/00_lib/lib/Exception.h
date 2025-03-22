#pragma once

#include <source_location>
#include <stdexcept>
#include <type_traits>

#include "00_lib/lib/String.h"

namespace nugdev::compiler::lib {

class Exception : public std::exception {
  public:
    Exception(const std::source_location &location, const String &message);
    const char *what() const noexcept override;

  private:
    String m_message;
    std::string m_message_str;
    std::source_location m_location;
};

#define DEFINE_DEFAULT_EXCEPTION_CONSTRUCTOR(CLASS)                                                                                                            \
    CLASS(const String &message, const std::source_location &location) : Exception(location, message) {}

#define DEFINE_DEFAULT_EXCEPTION_CONSTRUCTOR_WITH_MESSAGE(CLASS, MESSAGE)                                                                                      \
  public:                                                                                                                                                      \
    CLASS(const std::source_location &location) : Exception(location, MESSAGE) {}                                                                              \
    DEFINE_DEFAULT_EXCEPTION_CONSTRUCTOR(CLASS)

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
    DEFINE_DEFAULT_EXCEPTION_CONSTRUCTOR(OutOfRangeException)
};

class Nothing : public Exception {
  public:
    DEFINE_DEFAULT_EXCEPTION_CONSTRUCTOR_WITH_MESSAGE(Nothing, "Nothing")
};
#define TODO() throw_exception<Nothing>(std::source_location::current())

struct require {
    using self_t = require;

    bool m_condition;

    template <typename ExceptionType, typename... Args> self_t throws(Args &&...args) {
        if (m_condition == false) {
            throw_exception<ExceptionType>(std::forward<Args>(args)...);
        }
        return *this;
    }
};

struct InfiniteLoopDetectedException : public Exception {
    DEFINE_DEFAULT_EXCEPTION_CONSTRUCTOR_WITH_MESSAGE(InfiniteLoopDetectedException, "Infinite loop detected")
};

} // namespace nugdev::compiler::lib