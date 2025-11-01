//
// Created by nugde on 25. 10. 10..
//

#pragma once

#include <memory>

template <typename T>
class pointerable : public std::enable_shared_from_this<T> {
   private:
    std::shared_ptr<T> getSafeShared() noexcept {
        try {
            return this->shared_from_this();
        } catch (...) {
            return nullptr;
        }
    }

    std::shared_ptr<const T> getSafeShared() const noexcept {
        try {
            return this->shared_from_this();
        } catch (...) {
            return nullptr;
        }
    }

   public:
    // as()
    template <typename Child>
        requires std::is_base_of_v<T, Child>
    std::shared_ptr<Child> as() {
        auto self = getSafeShared();
        return self ? std::static_pointer_cast<Child>(self) : nullptr;
    }

    template <typename Child>
        requires std::is_base_of_v<T, Child>
    std::shared_ptr<const Child> as() const {
        auto self = getSafeShared();
        return self ? std::static_pointer_cast<const Child>(self) : nullptr;
    }

    // is() - 하나 이상의 타입 체크 (non-const)
    template <typename... Children>
        requires(sizeof...(Children) > 0) && (std::is_base_of_v<T, Children> && ...)
    bool is() {
        auto self = getSafeShared();
        if (!self)
            return false;

        // nullptr 체크를 명시적으로 추가
        return ((std::dynamic_pointer_cast<Children>(self) != nullptr) || ...);
    }

    // is() - 하나 이상의 타입 체크 (const)
    template <typename... Children>
        requires(sizeof...(Children) > 0) && (std::is_base_of_v<T, Children> && ...)
    bool is() const {
        auto self = getSafeShared();
        if (!self)
            return false;

        // nullptr 체크를 명시적으로 추가
        return ((std::dynamic_pointer_cast<const Children>(self) != nullptr) || ...);
    }

    // self()
    std::shared_ptr<T> self() {
        return getSafeShared();
    }
    std::shared_ptr<const T> self() const {
        return getSafeShared();
    }

    // is_valid()
    [[nodiscard]] bool is_valid() const {
        return getSafeShared() != nullptr;
    }

    template <typename Func>
    bool is_valid(Func condition) const {
        auto ptr = getSafeShared();
        return ptr && condition(ptr);
    }
};
