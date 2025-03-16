#include "02_parsing/ast/expression/type/TypeLiteralNodeParseStrategy.h"
#include "00_app/lib/UnicodeString.hpp"
#include "00_app/stream/StreamWorkbench.hpp"
#include "01_tokenize/Token.h"
#include "02_parsing/ast/expression/type/TypeLiteralNode.h"
#include <unordered_map>

namespace nugdev::compiler::ast::expression {

bool TypeLiteralNodeParseStrategy::can_parse(const tokenize::TokenStream &tokens) { return contains(tokens.current(), {tokenize::TokenType::Ident}); }

parsing::ParseStrategyResult TypeLiteralNodeParseStrategy::parse(const tokenize::TokenStream &tokens) {
    auto [node, itr] = stream::workbench(tokens, [this](auto &workbench) {
        // primitive type인지만 판단함. 나중에 객체 들어오면 조금 많이 복잡해질 예정.
        static const std::unordered_map<lib::String, size_t> primitiveTypeMap = {
            {u"i8", 1}, {u"i16", 2}, {u"i32", 4}, {u"i64", 8}, {u"u8", 1}, {u"u16", 2}, {u"u32", 4}, {u"u64", 8},
            {u"f8", 1}, {u"f16", 2}, {u"f32", 4}, {u"f64", 8}, {u"c8", 1}, {u"c16", 2}, {u"c32", 4}, {u"bool", 1},
        };
        static const std::unordered_map<lib::String, TypeMeta::PrimitiveType> primitiveTypeToEnumMap = {
            {u"i8", TypeMeta::PrimitiveType::int8},      {u"i16", TypeMeta::PrimitiveType::int16},    {u"i32", TypeMeta::PrimitiveType::int32},
            {u"i64", TypeMeta::PrimitiveType::int64},    {u"u8", TypeMeta::PrimitiveType::uint8},     {u"u16", TypeMeta::PrimitiveType::uint16},
            {u"u32", TypeMeta::PrimitiveType::uint32},   {u"u64", TypeMeta::PrimitiveType::uint64},   {u"f8", TypeMeta::PrimitiveType::float_8},
            {u"f16", TypeMeta::PrimitiveType::float_16}, {u"f32", TypeMeta::PrimitiveType::float_32}, {u"f64", TypeMeta::PrimitiveType::float_64},
            {u"c8", TypeMeta::PrimitiveType::char_8},    {u"c16", TypeMeta::PrimitiveType::char_16},  {u"c32", TypeMeta::PrimitiveType::char_32},
            {u"bool", TypeMeta::PrimitiveType::boolean},
        };

        auto token = workbench.current();
        auto it = primitiveTypeMap.find(token->get_literal());
        if (it == primitiveTypeMap.end()) {
            throw std::runtime_error("Invalid primitive type");
        }
        auto [type, size] = *it;
        auto meta = TypeMeta{
            .m_type = primitiveTypeToEnumMap.at(type),
            .m_size = size,
        };

        return create_node(*token, meta);
    });

    return {node, itr};
}

std::shared_ptr<ast::ASTNode> TypeLiteralNodeParseStrategy::create_node(const tokenize::Token &token, const TypeMeta &meta) {
    return std::make_shared<TypeLiteralNode>(token, meta);
}

} // namespace nugdev::compiler::ast::expression
