#pragma once

#include <memory>
#include <type_traits>

namespace nugdev::compiler::lib {

template <typename T> class PointerHelper : public std::enable_shared_from_this<T> {
  public:
    virtual ~PointerHelper() = default;

    template <typename Child>
        requires std::is_base_of_v<T, Child>
    std::shared_ptr<Child> as() {
        return std::static_pointer_cast<Child>(this->shared_from_this());
    }

    template <typename Child>
        requires std::is_base_of_v<T, Child>
    bool is() {
        try {
            auto ptr = std::dynamic_pointer_cast<Child>(this->shared_from_this());
            return (ptr != nullptr) ? true : false;
        } catch (std::bad_cast &e) {
            return false;
        }
    }

    std::shared_ptr<T> self() { return this->shared_from_this(); }

    bool isValid() const {
        return this->isValid([](auto ptr) { return ptr != nullptr; });
    }

    template <typename Func> bool isValid(Func condition) const { return condition(this->shared_from_this()); }
};

} // namespace nugdev::compiler::lib