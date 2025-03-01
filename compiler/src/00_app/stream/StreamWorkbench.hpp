#pragma once

#include <functional>

#include "Stream.hpp"

namespace nugdev::compiler::stream {

template <typename Type, typename Func>
auto workbench(const Stream<Type> &stream, Func &&func)
    -> std::tuple<decltype(std::forward<Func>(func)(std::declval<Stream<Type> &>())), typename Stream<Type>::iterator_t> {
    Stream<Type> workbench = stream.clone(); // 타입을 명시적으로 지정
    auto result = std::forward<Func>(func)(workbench);
    return std::make_tuple(result, stream.begin() + workbench.current().distance());
}

} // namespace nugdev::compiler::stream