#ifndef __STRING_LIBRARY__
#define __STRING_LIBRARY__

#include "String.h"

// clang-format off
#undef String
#define String(Instance) _Generic(&*(Instance), \
  chs           : StringConstructor_Chs,        \
  const_chs     : StringConstructor_Chs,        \
  wcs           : StringConstructor_Wcs,        \
  const_wcs     : StringConstructor_Wcs,        \
  unsigned int *: StringConstructor_Wcs,        \
  String        : StringConstructor_Str,        \
  String *      : StringConstructor_Strp,       \
  _Bool         : String_ToString_Bool,                \
  int           : String_ToString_Decimal,             \
  unsigned int  : String_ToString_Decimal_Unsigned,    \
  long          : String_ToString_Decimal,             \
  _int64        : String_ToString_Decimal,             \
  _uint64       : String_ToString_Decimal_Unsigned,    \
  float         : String_ToString_Digit,               \
  double        : String_ToString_Digit)               \
  (Instance)

#define toString(Instance) _Generic(&*(Instance), \
  _Bool         : String_ToString_Bool,                  \
  int           : String_ToString_Decimal,               \
  unsigned int  : String_ToString_Decimal_Unsigned,      \
  _int64        : String_ToString_Decimal,               \
  _uint64       : String_ToString_Decimal_Unsigned,      \
  double        : String_ToString_Digit,                 \
  (Instance)

String *String_ToString_Bool             (bool Value);
String *String_ToString_Decimal          (_int64 Value);
String *String_ToString_Decimal_Unsigned (_uint64 Value);
String *String_ToString_Digit            (double Value);

#define ValueOf(Type, Self) ValueOf_##Type(Self)
bool    String_ValueOf_Bool              (String *Self);
_int64  String_ValueOf_Decimal           (String *Self);
_uint64 String_ValueOf_Decimal_Unsigned  (String *Self);
double  String_ValueOf_Digit             (String *Self);

// clang-format on

#endif