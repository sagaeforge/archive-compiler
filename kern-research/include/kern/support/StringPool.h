#pragma once
#include "kern/support/Arena.h"
#include <string_view>
#include <unordered_set>
#include <cstring>

namespace kern {

// StringPool interns strings into an Arena, guaranteeing:
// - Identical strings return the same string_view (pointer equality)
// - All interned strings live as long as the Arena
// - O(1) amortized lookup via hash set
class StringPool {
public:
    explicit StringPool(Arena& arena) : arena_(arena) {}

    // Intern a single string. Returns a stable string_view.
    std::string_view intern(std::string_view s);

    // Intern the concatenation of two strings.
    std::string_view intern(std::string_view a, std::string_view b);

    // Intern the concatenation of three strings.
    std::string_view intern(std::string_view a, std::string_view b, std::string_view c);

    // Number of unique interned strings.
    size_t size() const { return table_.size(); }

    // Check if a string has been interned.
    bool contains(std::string_view s) const { return table_.count(s) > 0; }

private:
    // Copy string data into the arena and return a view into it.
    std::string_view arenaStore(const char* data, size_t len);

    Arena& arena_;
    std::unordered_set<std::string_view> table_;
};

} // namespace kern
