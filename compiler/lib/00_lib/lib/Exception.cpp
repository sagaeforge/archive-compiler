#include "00_lib/lib/Exception.h"

namespace nugdev::compiler::lib {

Exception::Exception(const std::source_location &location, const String &message) : m_message(message), m_location(location) {
    m_message_str = m_message.to_string();
}

const char *Exception::what() const noexcept {
    return m_message_str.c_str();
}

}  // namespace nugdev::compiler::lib
