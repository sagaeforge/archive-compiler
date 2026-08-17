#pragma once

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/stringbuffer.h>

#include <01_lib/Serializable/Serializable.hpp>

namespace nugdev::lib {

using JsonChar = rapidjson::UTF8<>;
using JsonAllocator = rapidjson::CrtAllocator;
using JsonDocument = rapidjson::GenericDocument<JsonChar, JsonAllocator>;
using JsonValue = rapidjson::GenericValue<JsonChar, JsonAllocator>;
using JsonArray = rapidjson::GenericArray<false, JsonValue>;
using JsonStringBuffer = rapidjson::GenericStringBuffer<JsonChar>;
using JsonWriter = rapidjson::Writer<JsonStringBuffer>;
using JsonPrettyWriter = rapidjson::PrettyWriter<JsonStringBuffer>;
using JsonStringRef = rapidjson::GenericStringRef<JsonChar>;

using rapidjson::Type;

template <typename Result>
using JsonSerializable = Serializable<Result, JsonValue>;

} // namespace nugdev::lib