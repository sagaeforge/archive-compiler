#include "RegisterTag.h"

namespace nugdev::compiler::generation {

RegisterTag::RegisterTag(const uuids::uuid &id) : m_id(id), m_hash(std::hash<uuids::uuid>{}(id)) {}

RegisterTag RegisterTag::create() noexcept {
    static std::mt19937 engine(std::random_device{}());
    static uuids::uuid_random_generator generator(engine);
    return RegisterTag(generator());
}

// 복사 생성자
RegisterTag::RegisterTag(const RegisterTag &other) : m_id(other.m_id), m_hash(other.m_hash) {}

// 이동 생성자
RegisterTag::RegisterTag(RegisterTag &&other) noexcept : m_id(std::move(other.m_id)), m_hash(other.m_hash) {}

// 이동 할당 연산자
RegisterTag &RegisterTag::operator=(RegisterTag &&other) noexcept {
    if (this != &other) {
        m_id = std::move(other.m_id);
        m_hash = other.m_hash;
    }
    return *this;
}

// 복사 할당 연산자
RegisterTag &RegisterTag::operator=(const RegisterTag &other) noexcept {
    if (this != &other) {
        m_id = other.m_id;
        m_hash = other.m_hash;
    }
    return *this;
}

// 3방향 비교 연산자 (C++20)
std::strong_ordering RegisterTag::operator<=>(const RegisterTag &other) const noexcept {
    // 해시값 먼저 비교
    if (m_hash < other.m_hash)
        return std::strong_ordering::less;
    if (m_hash > other.m_hash)
        return std::strong_ordering::greater;

    // 해시가 같으면 UUID 비교
    // uuids::uuid가 <=> 연산자를 지원하지 않을 수 있으므로 수동으로 비교
    auto this_bytes = m_id.as_bytes();
    auto other_bytes = other.m_id.as_bytes();

    for (size_t i = 0; i < 16; ++i) {
        if (this_bytes[i] < other_bytes[i])
            return std::strong_ordering::less;
        if (this_bytes[i] > other_bytes[i])
            return std::strong_ordering::greater;
    }

    return std::strong_ordering::equal;
}

std::size_t RegisterTag::hash() const { return m_hash; }

} // namespace nugdev::compiler::generation
