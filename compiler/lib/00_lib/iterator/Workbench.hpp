#pragma once

#include <functional>
#include <vector>

#include "00_lib/iterator/Context.hpp"
#include "00_lib/iterator/Iterator.hpp"
#include "00_lib/lib/Exception.h"

namespace nugdev::compiler::lib::iterator {

/**
 * @brief iterator 기반으로 동작을 간단하게 만들어주는 동작.
 * @note 절대로 멀티쓰레드에서 사용하지 마세요. 크리티컬 섹션이 너무 많아서 동작이 이상할 거임.
 * @tparam T
 */
template <typename T> class Workbench {
  public:
    using self_t = Workbench<T>;
    using elem_t = T;
    using iterator_t = iterator_t<T>;
    using context_t = context_t<T>;
    using command_t = command_t<T>;

  public:
    static self_t from(const std::vector<T> &vec) { return Workbench(context_t(vec)); }

  public:
    // 함수 객체 자체의 반환 타입을 추론
    template <typename Func> auto stream(const Func &converter) {
        require{m_context.valid()}.throws<ContextInvalidException>();

        // 함수 객체의 반환 타입을 추론하기 위한 타입 특성
        using result_type = std::invoke_result_t<Func, command_t>;

        std::vector<result_type> values;
        while (m_context.valid()) {
            auto prev = m_context.current();

            // 먼저 ContextCommand 클래스에 적절한 생성자가 있다고 가정
            auto result = converter(command_t{m_context});

            if (m_context.valid() && prev.position() == m_context.current().position()) {
                throw_exception<InfiniteLoopDetectedException>();
            }
            values.push_back(result);
        }
        return values;
    }

  private:
    Workbench(const context_t &context) : m_context{context} {}

  private:
    context_t m_context;
};

} // namespace nugdev::compiler::lib::iterator