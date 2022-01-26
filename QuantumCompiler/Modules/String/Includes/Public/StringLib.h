#ifndef __STRING_LIBRARY__
#define __STRING_LIBRARY__

#include "String.h"
#include "StringAry.h"

// clang-format off
#define toString(Instance) _Generic((Instance),                 \
  _Bool               : String_ToString_Bool,                   \
  int                 : String_ToString_Decimal,                \
  unsigned int        : String_ToString_Decimal_Unsigned,       \
  long                : String_ToString_Decimal,                \
  long long           : String_ToString_Decimal,                \
  unsigned long long  : String_ToString_Decimal_Unsigned,       \
  double              : String_ToString_Digit                   \
  )(Instance)

#define ValueOf(Instance) _Generic((Instance),                \
  _Bool               : String_ValueOf_Bool,                  \
  short               : String_ValueOf_Decimal,               \
  int                 : String_ValueOf_Decimal,               \
  long                : String_ValueOf_Decimal,               \
  long long           : String_ValueOf_Decimal,               \
  unsigned short      : String_ValueOf_Decimal_Unsigned,      \
  unsigned int        : String_ValueOf_Decimal_Unsigned,      \
  unsigned long       : String_ValueOf_Decimal_Unsigned,      \
  unsigned long long  : String_ValueOf_Decimal_Unsigned,      \
  float               : String_ValueOf_Digit,                 \
  double              : String_ValueOf_Digit                  \
  ) (Instance)
// clang-format on

String
String_ToString_Bool(bool Value);
String
String_ToString_Decimal(int64_t Value);
String
String_ToString_Decimal_Unsigned(uint64_t Value);
String
String_ToString_Digit(double Value);

bool
String_ValueOf_Bool(String Self);
int64_t
String_ValueOf_Decimal(String Self);
uint64_t
String_ValueOf_Decimal_Unsigned(String Self);
double
String_ValueOf_Digit(String Self);

#endif