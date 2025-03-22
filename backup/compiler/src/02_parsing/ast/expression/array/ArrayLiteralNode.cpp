#include "ArrayLiteralNode.h"
#include "02_parsing/TypeMeta.h"

namespace nugdev::compiler::ast::expression {

ArrayLiteralNode::ArrayLiteralNode(const tokenize::Token &token, const std::vector<std::shared_ptr<Expression>> &elements)
    : m_token(token), m_elements(elements) {}

const tokenize::Token &ArrayLiteralNode::get_token() const { return m_token; }

const std::vector<std::shared_ptr<Expression>> &ArrayLiteralNode::get_elements() const { return m_elements; }

TypeInfo ArrayLiteralNode::get_type_info() const {
    if (m_elements.empty()) {
        return TypeInfo{.m_size = 0, .m_is_primitive = false, .m_literal = u"array<>"};
    }

    auto type = m_elements.front()->get_type_info();
    return TypeInfo{.m_size = type.m_size * static_cast<std::uint32_t>(m_elements.size()),
                    .m_is_primitive = type.m_is_primitive,
                    .m_literal = u"array<" + type.m_literal + u">"};
}

} // namespace nugdev::compiler::ast::expression
