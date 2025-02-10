#include "OperatorTokenFactory.h"

namespace nugdev::compiler::tokenize {

OperatorTokenFactory::OperatorTokenFactory() {
    operatorMap[L'+'] = TokenType::Plus;
    operatorMap[L'-'] = TokenType::Minus;
    operatorMap[L'*'] = TokenType::Asterisk;
    operatorMap[L'/'] = TokenType::Slash;
    operatorMap[L'='] = TokenType::Assign;
    operatorMap[L'<'] = TokenType::LessThan;
    operatorMap[L'>'] = TokenType::GreaterThan;
    operatorMap[L'('] = TokenType::LParen;
    operatorMap[L')'] = TokenType::RParen;
    operatorMap[L'{'] = TokenType::LBrace;
    operatorMap[L'}'] = TokenType::RBrace;
    operatorMap[L'['] = TokenType::LBracket;
    operatorMap[L']'] = TokenType::RBracket;
    operatorMap[L','] = TokenType::Comma;
    operatorMap[L';'] = TokenType::SemiColon;
    operatorMap[L':'] = TokenType::Colon;
}

bool OperatorTokenFactory::canHandle(wchar_t ch) { return operatorMap.find(ch) != operatorMap.end(); }

Token OperatorTokenFactory::createToken(std::wistream &stream) { return Token::from(operatorMap[stream.get()], stream.get()); }

} // namespace nugdev::compiler::tokenize