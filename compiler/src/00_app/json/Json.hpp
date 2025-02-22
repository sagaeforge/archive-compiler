#pragma once

#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>

namespace nugdev::compiler::json {

using JsonAllocator = rapidjson::CrtAllocator;
using JsonDocument = rapidjson::GenericDocument<rapidjson::UTF8<>, JsonAllocator>;
using JsonValue = rapidjson::GenericValue<rapidjson::UTF8<>, JsonAllocator>;

using Type = rapidjson::Type;

} // namespace nugdev::compiler::json