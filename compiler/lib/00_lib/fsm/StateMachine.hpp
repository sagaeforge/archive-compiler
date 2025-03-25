#pragma once

#include "00_lib/fsm/StateTag.h"
#include "00_lib/fsm/State.hpp"
#include "00_lib/fsm/Transition.hpp"
#include "00_lib/lib/Exception.h"

#include <map>
#include <algorithm>
#include <any>

namespace nugdev::compiler::lib::fsm {

class StateMachineException : public lib::Exception {
    DEFINE_DEFAULT_EXCEPTION_CONSTRUCTOR_WITH_MESSAGE(StateMachineException, Exception, "StateMachine Exception")
};

class StateMachineEdgeNotFoundException : public StateMachineException {
    DEFINE_DEFAULT_EXCEPTION_CONSTRUCTOR_WITH_MESSAGE(StateMachineEdgeNotFoundException, StateMachineException, "StateMachine Edge Not Found Exception")
};

class StateMachineMultipleEdgeException : public StateMachineException {
    DEFINE_DEFAULT_EXCEPTION_CONSTRUCTOR_WITH_MESSAGE(StateMachineMultipleEdgeException, StateMachineException, "StateMachine Multiple Edge Exception")
};

template <typename Type>
struct StateMachineContext {
private:
    class Iterator {};
    using iterator_t = Iterator;

private:
    iterator_t m_current;
    std::vector<Type> m_values;
};

template <typename Return>
struct StateMachineResult {
    std::optional<lib::Exception> exception;
    std::vector<Return> values;

    using self_t = StateMachineResult;

    static self_t success(const std::vector<Return> &values) {
        return self_t{.exception = std::nullopt, .values = values};
    }
    static self_t failure(const lib::Exception &exception) {
        return self_t{.exception = exception, .values = {}};
    }
};

template <typename Type, typename Return, typename Context = StateMachineContext<Type>>
class StateMachine {
public:
    using self_t = StateMachine;
    using elem_t = Type;
    using return_t = Return;
    using context_t = Context;
    using state_tag_t = StateTag;
    using state_t = State<std::variant<return_t, state_t>>(const self_t &) > ;
    using transition_tag_t = TransitionTag;
    using transition_t = Transition<context_t>;
    using result_t = StateMachineResult<return_t>;

public:
    /**
     * @brief 상태 머신을 실행합니다.
     * @note 상태 머신을 실행하면, 현재 상태가 변경되며, 상태 머신이 종료될 때까지 반복합니다.
     */
    result_t run() {
        std::vector<return_t> returns;

        auto m_current = start_state();
        try {
            while (m_current != end_state()) {
                // 현재 상태에서 갈 수 있는 간선들을 찾는다.
                auto transitions = m_transitions[m_current];

                // 갈 수 있는 간선들을 평가한다.
                std::vector<state_tag_t> nextStageTags;
                for (auto transition : transitions) {
                    if (transition.evaluate()) {
                        nextStageTags.push_back(transition.get_target());
                    }
                }

                // 3가지 상황이 있는데,
                // 1. 갈 수 있는 간선이 없다. - 그러면 이제 이전 노드로 결과를 반환해야 하지 않나?라고 생각 듬.
                // 2. 갈 수 있는 간선이 하나다.
                // 3. 갈 수 있는 간선이 여러개다.
                if (nextStageTags.empty()) {
                    throw_exception<StateMachineEdgeNotFoundException>();
                } else if (nextStageTags.size() == 1) {
                    m_current = nextStageTags[0];
                    auto result = run_state(m_current, m_context);
                    returns.push_back(result);
                } else {
                    throw_exception<StateMachineMultipleEdgeException>();
                }
            }
        } catch (lib::Exception &e) {
            return result_t::failure(e);
        }
        return result_t::success(returns);
    }

public:
    std::map<state_tag_t, std::tuple<state_t, std::vector<transition_t>>> &get_states() {
        std::map<state_tag_t, std::tuple<state_t, std::vector<transition_t>>> states;
        for (auto &[state_tag, state] : m_states) {
            states[state_tag] = std::make_tuple(state, m_transitions[state_tag]);
        }
        return states;
    }

protected:
    StateMachine() {
    }

protected:
    /**
     * @brief 시작 노드를 정의합니다.
     * @return state_tag_t
     */
    virtual state_tag_t start_state() = 0;

    /**
     * @brief 종료 노드를 정의합니다.
     * @return state_tag_t
     */
    virtual state_tag_t end_state() = 0;

protected:
    void add_state(const State<return_t(context_t)> &state) {
        m_states[state.get_tag()] = state;
    }

    void add_transition(const Transition<context_t> &transition) {
        m_transitions[transition.get_source()].push_back(transition);
    }

private:
    return_t run_state(state_tag_t state_tag, const context_t &context) {
        return m_states[state_tag](*this);
    }

private:
    state_tag_t m_current;
    context_t m_context;
    std::map<state_tag_t, state_t> m_states;
    std::map<state_tag_t, std::vector<transition_t>> m_transitions;
};
}