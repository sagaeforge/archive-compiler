#pragma once

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/stringbuffer.h>

namespace nugdev::compiler::json {

using JsonChar = rapidjson::UTF8<>;
using JsonAllocator = rapidjson::CrtAllocator;
using JsonDocument = rapidjson::GenericDocument<JsonChar, JsonAllocator>;
using JsonValue = rapidjson::GenericValue<JsonChar, JsonAllocator>;
using JsonStringBuffer = rapidjson::StringBuffer;
using JsonFormatter = rapidjson::PrettyWriter<JsonStringBuffer>;

using rapidjson::Type;
} // namespace nugdev::compiler::json