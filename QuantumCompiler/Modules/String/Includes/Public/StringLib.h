#ifndef __STRING_LIBRARY__
#define __STRING_LIBRARY__

#include <String.h>
#include <StringAry.h>

// clang-format off
#define toString(Instance, args...) _Generic((Instance),         \
  _Bool               : String_ToString_Bool,                    \
  int                 : String_ToString_Decimal,                 \
  unsigned int        : String_ToString_Decimal_Unsigned,        \
  long                : String_ToString_Decimal,                 \
  long long           : String_ToString_Decimal,                 \
  unsigned long long  : String_ToString_Decimal_Unsigned,        \
  double              : String_ToString_Digit,                   \
  StringAry           : String_ToString_StringAry)               \
  (Instance, ##args)
// clang-format on

#define ValueOf(DataType) (DataType(*)(String)) String_ValueOfSearch(#DataType)

Func_t
String_ValueOfSearch(const char* DataType);

String
String_ToString_Bool(bool Value);
String
String_ToString_Decimal(int64_t Value);
String
String_ToString_Decimal_Unsigned(uint64_t Value);
String
String_ToString_Digit(double Value);
String
String_ToString_StringAry(StringAry Value, String ReplaceWord);

bool
String_ValueOf_Bool(String Self);
int64_t
String_ValueOf_Decimal(String Self);
uint64_t
String_ValueOf_Decimal_Unsigned(String Self);
double
String_ValueOf_Digit(String Self);

#endif