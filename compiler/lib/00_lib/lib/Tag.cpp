#include "00_lib/lib/Tag.h"

#include <uuid.h>

namespace nugdev::compiler::lib {

Tag::Tag() : m_id(), m_hash() {
    static std::mt19937 engine(std::random_device{}());
    static uuids::uuid_random_generator generator(engine);
    m_id = generator();
    m_hash = std::hash<uuids::uuid>{}(m_id);
}

Tag::Tag(const lib::String &str) : m_id(), m_hash() {
    auto uuid = uuids::uuid::from_string(str.to_string());
    if (uuid.has_value()) {
        m_id = uuid.value();
        m_hash = std::hash<uuids::uuid>{}(m_id);
    } else {
        throw std::runtime_error("Invalid UUID string");
    }
}

// 복사 생성자
Tag::Tag(const Tag &other) : m_id(other.m_id), m_hash(other.m_hash) {
}

// 이동 생성자
Tag::Tag(Tag &&other) noexcept : m_id(std::move(other.m_id)), m_hash(other.m_hash) {
}

// 이동 할당 연산자
Tag &Tag::operator=(Tag &&other) noexcept {
    if (this != &other) {
        m_id = std::move(other.m_id);
        m_hash = other.m_hash;
    }
    return *this;
}

// 복사 할당 연산자
Tag &Tag::operator=(const Tag &other) noexcept {
    if (this != &other) {
        m_id = other.m_id;
        m_hash = other.m_hash;
    }
    return *this;
}

Tag &Tag::operator=(const lib::String &str) {
    auto rhs = Tag(str);
    if (this != &rhs) {
        m_id = rhs.m_id;
        m_hash = rhs.m_hash;
    }
    return *this;
}

bool Tag::operator==(const Tag &other) const noexcept {
    return m_id == other.m_id;
}

// 3방향 비교 연산자 (C++20)
std::strong_ordering Tag::operator<=>(const Tag &other) const noexcept {
    // 해시값 먼저 비교
    if (m_hash < other.m_hash) return std::strong_ordering::less;
    if (m_hash > other.m_hash) return std::strong_ordering::greater;

    // 해시가 같으면 UUID 비교
    // uuids::uuid가 <=> 연산자를 지원하지 않을 수 있으므로 수동으로 비교
    auto this_bytes = m_id.as_bytes();
    auto other_bytes = other.m_id.as_bytes();

    for (size_t i = 0; i < 16; ++i) {
        if (this_bytes[i] < other_bytes[i]) return std::strong_ordering::less;
        if (this_bytes[i] > other_bytes[i]) return std::strong_ordering::greater;
    }

    return std::strong_ordering::equal;
}

std::size_t Tag::hash() const {
    return m_hash;
}

lib::String Tag::to_str() const {
    return lib::String(uuids::to_string(m_id));
}

}  // namespace nugdev::compiler::lib
