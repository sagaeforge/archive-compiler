#include "00_lib/lib/Json.hpp"

namespace nugdev::compiler::lib {

JsonValue create_json_value(const String &str, JsonAllocator &allocator) {
    auto value = JsonValue(Type::kStringType);
    value.SetString(str.to_string().c_str(), str.to_string().length(), allocator);
    return value;
}

} // namespace nugdev::compiler::lib
