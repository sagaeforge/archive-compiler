#pragma once

#include "00_lib/fsm/StateTag.h"
#include "00_lib/fsm/TransitionTag.h"

namespace nugdev::compiler::lib::fsm {

/**
 * @brief 상태 간의 간선.
 * @note 상태 간의 간선은 무조건 존재하며, 내부의 평가 결과에 따라, 상태가 변경되는 구조임.
 */
template <typename... Args>
class Transition {
public:
    using self_t = Transition;
    using state_tag_t = StateTag;

public:
    virtual bool evaluate(Args... args) = 0;

private:
    TransitionTag m_tag;
    state_tag_t m_source;
    state_tag_t m_target;
};

}