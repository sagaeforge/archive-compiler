#pragma once

#include <optional>

namespace nugdev::compiler::lib {

template <typename Type> class Strategy;

template <typename ReturnType, typename... Args> class Strategy<ReturnType(Args...)> {
  public:
    virtual bool can_handle(Args... args) = 0;
    virtual std::optional<ReturnType> handle(Args... args) = 0;

    std::optional<ReturnType> operator()(Args... args) { return handle(args...); }
};
template <typename... Args> class Strategy<void(Args...)> {
  public:
    virtual bool can_handle(Args... args) = 0;
    virtual void handle(Args... args) = 0;

    void operator()(Args... args) { handle(args...); }
};

} // namespace nugdev::compiler::lib
