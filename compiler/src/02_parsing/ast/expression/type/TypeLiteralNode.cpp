#include "02_parsing/ast/expression/type/TypeLiteralNode.h"

namespace nugdev::compiler::ast::expression {

TypeLiteralNode::TypeLiteralNode(const tokenize::Token &token, const TypeMeta &meta) : m_token(token), m_meta(meta) {}

const tokenize::Token &TypeLiteralNode::get_token() const { return m_token; }

const TypeMeta &TypeLiteralNode::get_meta() const { return m_meta; }

} // namespace nugdev::compiler::ast::expression
