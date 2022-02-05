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
  unsigned long       : String_ToString_Decimal_Unsigned,        \
  long long           : String_ToString_Decimal,                 \
  unsigned long long  : String_ToString_Decimal_Unsigned,        \
  double              : String_ToString_Digit,                   \
  const StringAry     : String_ToString_StringAry,               \
  StringAry           : String_ToString_StringAry)               \
  (Instance, ##args)

#define ValueOf(DataType) (DataType(*)(String)) String_ValueOfSearch(#DataType)

Func_t String_ValueOfSearch(const char* DataType);

String    String_ToString_Bool              (bool pValue);
String    String_ToString_Decimal           (int64_t pValue);
String    String_ToString_Decimal_Unsigned  (uint64_t pValue);
String    String_ToString_Digit             (double pValue);
String    String_ToString_StringAry         (StringAry pValue, String pReplaceWord);
bool      String_ValueOf_Bool               (String pSelf);
int64_t   String_ValueOf_Decimal            (String pSelf);
uint64_t  String_ValueOf_Decimal_Unsigned   (String pSelf);
double    String_ValueOf_Digit              (String pSelf);

extern struct StringLibMethod StringLibMethod;

#endif