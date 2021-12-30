#ifndef __STRING_LIBRARY__
#define __STRING_LIBRARY__

#include "String.h"

// clang-format off
#undef String
#define String(Instance)                        \
  Method_Combine(_Generic((Instance),           \
  chs           : StringConstructor_Chs,        \
  const_chs     : StringConstructor_Chs,        \
  wcs           : StringConstructor_Wcs,        \
  const_wcs     : StringConstructor_Wcs,        \
  String        : StringConstructor_Str,        \
  String *      : StringConstructor_Strp,       \
  _Bool         : ToString_Bool,                \
  int           : ToString_Decimal,             \
  unsigned int  : ToString_Decimal_Unsigned,    \
  long          : ToString_Decimal,             \
  _int64        : ToString_Decimal,             \
  _uint64       : ToString_Decimal_Unsigned,    \
  float         : ToString_Digit,               \
  double        : ToString_Digit),              \
  Instance)

#define toString(Instance)                      \
  Method_Combine(_Generic((Instance),           \
  _Bool         : ToString_Bool,                \
  int           : ToString_Decimal,             \
  unsigned int  : ToString_Decimal_Unsigned,    \
  _int64        : ToString_Decimal,             \
  _uint64       : ToString_Decimal_Unsigned,    \
  double        : ToString_Digit,               \
  ), Instance)
String *ToString_Bool             (bool Value);
String *ToString_Decimal          (_int64 Value);
String *ToString_Decimal_Unsigned (_uint64 Value);
String *ToString_Digit            (double Value);

#define ValueOf(Type, Self) ValueOf_##Type(Self)
bool    ValueOf_Bool              (String *Self);
_int64  ValueOf_Decimal           (String *Self);
_uint64 ValueOf_Decimal_Unsigned  (String *Self);
double  ValueOf_Digit             (String *Self);

// clang-format on
extern struct StringLibMethod StringLibMethod;

#endif