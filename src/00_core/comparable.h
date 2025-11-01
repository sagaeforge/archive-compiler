//
// Created by nugde on 25. 10. 8..
//

#pragma once

#include <compare>
#include <concepts>
#include <type_traits>
#include <typeinfo>

template <typename Derived>
class comparable {
   public:
    virtual ~comparable() = default;

    friend std::partial_ordering operator<=>(const Derived& lhs, const Derived& rhs) {
        return lhs.compare(rhs);
    }

    friend bool operator==(const Derived& lhs, const Derived& rhs) {
        return lhs.compare(rhs) == std::partial_ordering::equivalent;
    }

    virtual std::partial_ordering compare(const Derived& other) const = 0;

   protected:
    [[nodiscard]] bool is_same_type(const Derived& other) const noexcept {
        return typeid(*static_cast<const Derived*>(this)) == typeid(other);
    }
};

// 5. Concept로 타입 제약 추가 (C++20)
template <typename T>
concept ComparableType = std::derived_from<T, comparable<T> > && requires(const T& a, const T& b) {
    { a.compare(b) } -> std::same_as<std::partial_ordering>;
};
