#include "kern/support/StringPool.h"
#include <cstring>

namespace kern {

std::string_view StringPool::intern(std::string_view s) {
    auto it = table_.find(s);
    if (it != table_.end()) {
        return *it;
    }
    auto interned = arenaStore(s.data(), s.size());
    table_.insert(interned);
    return interned;
}

std::string_view StringPool::intern(std::string_view a, std::string_view b) {
    size_t total = a.size() + b.size();
    char* buf = static_cast<char*>(arena_.allocate(total, 1));
    std::memcpy(buf, a.data(), a.size());
    std::memcpy(buf + a.size(), b.data(), b.size());
    std::string_view combined(buf, total);

    auto it = table_.find(combined);
    if (it != table_.end()) {
        // Already interned — the arena allocation is wasted but harmless
        return *it;
    }
    table_.insert(combined);
    return combined;
}

std::string_view StringPool::intern(std::string_view a, std::string_view b, std::string_view c) {
    size_t total = a.size() + b.size() + c.size();
    char* buf = static_cast<char*>(arena_.allocate(total, 1));
    std::memcpy(buf, a.data(), a.size());
    std::memcpy(buf + a.size(), b.data(), b.size());
    std::memcpy(buf + a.size() + b.size(), c.data(), c.size());
    std::string_view combined(buf, total);

    auto it = table_.find(combined);
    if (it != table_.end()) {
        return *it;
    }
    table_.insert(combined);
    return combined;
}

std::string_view StringPool::arenaStore(const char* data, size_t len) {
    char* buf = static_cast<char*>(arena_.allocate(len, 1));
    std::memcpy(buf, data, len);
    return std::string_view(buf, len);
}

} // namespace kern
