#pragma once

namespace nugdev::compiler::tokenize {

enum class TokenType {
  Illegal = 0,
  Number = 2,
  Identifier = 3,
  String = 4,
  Boolean = 5,
  Null = 6,
  True = 7,
  False = 8,

  // Special characters
  Plus = '+',
  Minus = '-',
  Asterisk = '*',
  Slash = '/',
  Percent = '%',
  Exclamation = '!',
  Question = '?',
  Dot = '.',
  Comma = ',',
  Semicolon = ';',
  Colon = ':',
  LeftParen = '(',
  RightParen = ')',
  LeftBrace = '{',
  RightBrace = '}',
  LeftBracket = '[',
  RightBracket = ']',
  Ampersand = '&',
  Pipe = '|',
  Caret = '^',
  Tilde = '~',
  At = '@',
  Dollar = '$',
  Backslash = '\\',
  SingleQuote = '\'',
  DoubleQuote = '"',
  Backtick = '`',

  // 대입 연산자
  Assign = '=',
  PlusAssign = 128,      //'+=',
  MinusAssign = 129,     //'-=',
  AsteriskAssign = 130,  //'*=',
  SlashAssign = 131,     //'/=',
  PercentAssign = 132,   //'%=',
  AmpersandAssign = 133, //'&=',
  PipeAssign = 134,      //'|=',
  CaretAssign = 135,     //'^=',
  TildeAssign = 136,     //'~=',

  // 증감 연산자
  Increment = 137, //'++',
  Decrement = 138, //'--',

  // 비교 연산자
  Equal = 139,            //'==',
  NotEqual = 140,         //'!='
  LessThanEqual = 141,    //'<='
  GreaterThanEqual = 142, //'>='
  LessThan = '<',
  GreaterThan = '>',

  // 논리 연산자
  LogicalAnd = 145, //'and'
  LogicalOr = 146,  //'or'
  LogicalNot = 147, //'not'

  // 비트 연산자
  // BitwiseAnd = 148,        //'&'
  // BitwiseOr = 149,         //'|'
  // BitwiseXor = 150,        //'^'
  // BitwiseNot = 151,        //'~'
  BitwiseShiftLeft = 152,  //'<<'
  BitwiseShiftRight = 153, //'>>'

  // null 연산자
  NullElvis = 154,      //'?:'
  NullAssertion = 155,  //'!!'
  NullSafeAccess = 156, //'?.'

  // 키워드
  Let = 158,      //'let'
  Mut = 159,      //'mut'
  If = 160,       //'if'
  Elif = 161,     //'elif'
  Else = 162,     //'else'
  For = 163,      //'for'
  Break = 164,    //'break'
  Continue = 165, //'continue'
  Function = 166, //'function'
  Return = 167,   //'return'
  When = 168,     //'when'
};

}