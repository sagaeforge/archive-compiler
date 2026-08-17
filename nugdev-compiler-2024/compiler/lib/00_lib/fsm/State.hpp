#pragma once

#include "00_lib/fsm/StateTag.h"

namespace nugdev::compiler::lib::fsm {

template <typename T>
class State;

template <typename Return, typename... Args>
class State<Return(Args...)> {
public:
    using self_t = State;
    using state_tag_t = StateTag;

public:
    virtual Return handle(Args... args) = 0;
    virtual Return operator()(Args... args) {
        return handle(args...);
    }

private:
    state_tag_t m_tag;
};

// 상태와 간선 개념이 중요할 것 같긴함.

}
