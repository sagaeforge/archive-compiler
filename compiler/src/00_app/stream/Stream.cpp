#include "00_app/stream/Stream.hpp"

namespace nugdev::compiler::stream {

StringStream make_stream(const icu::UnicodeString &str) {
    std::vector<char16_t> elems;
    for (int32_t i = 0; i < str.length(); i++) {
        elems.push_back(str.charAt(i));
    }
    return Stream<char16_t>(elems);
}
} // namespace nugdev::compiler::stream
