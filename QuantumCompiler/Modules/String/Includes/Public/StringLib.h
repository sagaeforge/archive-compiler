#ifndef __STRING_LIBRARY__
#define __STRING_LIBRARY__

#include "String.h"

// clang-format off

#define toString(Instance)                      \
  Method_Combine(_Generic((Instance),           \
  _Bool         : ToString_Bool,                \
  int           : ToString_Decimal,             \
  unsigned int  : ToString_Decimal_unsigned,    \
  _int64        : ToString_Decimal,             \
  _uint64       : ToString_Decimal_unsigned,    \
  double        : ToString_Digit,               \
  ), Instance)
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
  unsigned int  : ToString_Decimal_unsigned,    \
  _int64        : ToString_Decimal,             \
  _uint64       : ToString_Decimal_unsigned,    \
  float         : ToString_Digit,               \
  double        : ToString_Digit),              \
  Instance)
// clang-format on

String *ToString_Bool(bool Value);
String *ToString_Decimal(_int64 Value);
String *ToString_Decimal_unsigned(_uint64 Value);
String *ToString_Digit(double Value);

#endif