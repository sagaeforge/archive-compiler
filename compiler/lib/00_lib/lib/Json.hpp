#pragma once

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/stringbuffer.h>

#include "00_lib/lib/String.h"
#include "00_lib/lib/Transformer.hpp"

namespace nugdev::compiler::lib {

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

template <typename T>
class JsonSerializer {
public:
    virtual std::optional<JsonValue> serialize(const T &value, JsonAllocator &allocator) = 0;
    virtual std::optional<JsonValue> serialize(const std::vector<T> &value, JsonAllocator &allocator) = 0;
    virtual ~JsonSerializer() = default;
};

template <typename T>
class JsonDeserializer {
public:
    virtual std::optional<T> deserialize(const JsonValue &value) = 0;
    virtual std::optional<std::vector<T>> deserialize(const JsonArray &value) = 0;
    virtual ~JsonDeserializer() = default;
};

JsonValue create_json_value(const String &str, JsonAllocator &allocator);

}  // namespace nugdev::compiler::lib