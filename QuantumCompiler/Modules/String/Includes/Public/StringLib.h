#ifndef __STRING_LIBRARY__
#define __STRING_LIBRARY__

#include "String.h"
#include "StringAry.h"

// clang-format off
#define toString(Instance) _Generic((Instance),           \
  _Bool         : String_ToString_Bool,                   \
  int           : String_ToString_Decimal,                \
  unsigned int  : String_ToString_Decimal_Unsigned,       \
  long          : String_ToString_Decimal,                \
  _int64        : String_ToString_Decimal,                \
  _uint64       : String_ToString_Decimal_Unsigned,       \
  double        : String_ToString_Digit                   \
  )(Instance)
// clang-format on

String*
String_ToString_Bool(bool Value);
String*
String_ToString_Decimal(_int64 Value);
String*
String_ToString_Decimal_Unsigned(_uint64 Value);
String*
String_ToString_Digit(double Value);

#define ValueOf(Type, Self) ValueOf_##Type(Self)
bool
String_ValueOf_Bool(String* Self);
_int64
String_ValueOf_Decimal(String* Self);
_uint64
String_ValueOf_Decimal_Unsigned(String* Self);
double
String_ValueOf_Digit(String* Self);

// clang-format on

#endif