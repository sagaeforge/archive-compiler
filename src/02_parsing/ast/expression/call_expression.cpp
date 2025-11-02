//
// Created by lambda on 11/1/25.
//

#include "call_expression.h"

#include "02_parsing/ast/util/visitor.h"

CallExpression::CallExpression(const Token &token, const Node<Expression> &callee,
                               const std::vector<Node<Expression> > &args) : m_token(token),
                                                                             m_callee(callee),
                                                                             m_args(args) {
}

std::partial_ordering CallExpression::compare(const std::shared_ptr<ASTNode> &other) const {
    auto otherNode = other->as<CallExpression>();
    if (otherNode == nullptr) {
        return std::partial_ordering::unordered;
    }

    if (m_callee->compare(otherNode->m_callee) != std::partial_ordering::equivalent) {
        return m_callee->compare(otherNode->m_callee);
    }

    if (m_args.size() != otherNode->m_args.size()) {
        return std::partial_ordering::unordered;
    }

    for (size_t i = 0; i < m_args.size(); ++i) {
        auto arg = m_args[i];
        auto otherArg = otherNode->m_args[i];
        if (arg->compare(otherArg) != std::partial_ordering::equivalent) {
            return std::partial_ordering::unordered;
        }
    }

    return std::partial_ordering::equivalent;
}

void CallExpression::accept(ASTVisitor &visitor) const {
    visitor.visit(std::static_pointer_cast<const CallExpression>(self()));
}

Token CallExpression::token() const {
    return m_callee->token();
}

Node<Expression> CallExpression::callee() const {
    return m_callee;
}

std::vector<Node<Expression> > CallExpression::args() const {
    return m_args;
}
