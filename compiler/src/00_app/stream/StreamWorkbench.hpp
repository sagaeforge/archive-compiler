#pragma once

#include "Stream.hpp"

#include <functional>

namespace nugdev::compiler::stream {

template <typename Type, typename Func>
auto workbench(const Stream<Type> &stream, Func func) -> std::tuple<decltype(func(std::declval<Stream<Type> &>())), typename Stream<Type>::iterator_t> {
    auto workbench = stream.clone();
    auto result = func(workbench);
    return std::make_tuple(result, stream.begin() + workbench.current().distance());
}

} // namespace nugdev::compiler::stream