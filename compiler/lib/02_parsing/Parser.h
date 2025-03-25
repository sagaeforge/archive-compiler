#pragma once

#include "00_lib/lib/Exception.h"
#include "00_lib/lib/Tag.h"
#include "02_parsing/ast/ASTNode.h"

#include <map>

namespace nugdev::compiler::parsing {

class ParsingStateTag : public lib::Tag {
    using Tag::Tag;
};
class ParsingTransitionTag : public lib::Tag {
    using Tag::Tag;
};

struct ParsingContext {
private:
    class Iterator {};
    using iterator_t = Iterator;

private:
    iterator_t m_current;
    ast::ASTNodePtr m_node;
    std::vector<ast::ASTNodePtr> m_values;
};

// transition에 영향을 주는 값이 있어야 하긴 함.
class ParsingResult {};

class ParsingState {};
class ParsingTransition {
public:
    using self_t = ParsingTransition;
    using state_tag_t = ParsingStateTag;
    using transition_tag_t = ParsingTransitionTag;

public:
    virtual bool evaluate(const tokenize::TokenType &token_type) = 0;

private:
    transition_tag_t m_tag;
    state_tag_t m_source;
    state_tag_t m_target;
};
class AlwaysTrueTransition : public ParsingTransition {
public:
    bool evaluate(const tokenize::TokenType &token_type) override {
        return true;
    }
};

/**
 * @brief FSM 기반 파서
 */
class Parser {
public:
    using self_t = Parser;
    using context_t = ParsingContext;
    using state_tag_t = ParsingStateTag;
    using transition_tag_t = ParsingTransitionTag;
    using state_t = ParsingState;
    using transition_t = ParsingTransition;
    using result_t = ParsingResult;

public:
    ast::ASTNodePtr parse(const std::vector<tokenize::Token> &tokens) {
        auto current = m_startState;
        while (current != m_endState) {
            auto result = run_state(current);
        }
    }

private:
    result_t run_state(state_tag_t state_tag) {
        return m_states[state_tag](*this);
    }

private:
    context_t m_context;
    state_tag_t m_startState;
    state_tag_t m_endState;
    std::map<state_tag_t, state_t> m_states;
    std::map<state_tag_t, std::vector<transition_t>> m_transitions;
};

}  // namespace nugdev::compiler

// 예시 상황 1 + 1이라고 했을 때 순회 순서
/*
  program(1)
   - statements(2)
     - expresison(3)
       - infix_expression(4)
         - right(5)
           - number(6)
         - left(7)
           - number(8)
         - operator(9)
           - +

    이제 이를 state로 표현하면
    program_root(노드 생성)
        program_statements(노드 수정)
            expression_root(노드 생성)
                infix_expression_root(노드 생성)
                    right_expression(노드 생성)
                        number_expression(노드 생성)
                    left_expression(노드 생성)
                        number_expression(노드 생성)
                    operator(값)
                    infix_expression_update(노드 수정)

 */