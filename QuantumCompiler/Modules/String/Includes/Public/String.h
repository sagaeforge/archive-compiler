
#ifndef __STRING__
#define __STRING__

#include "Types/DataTypes_String.h"

// TODO 자료형 확장으로 메소드 기능 구현하기
// TODO format, parttern, is 계열 함수 구현

#define String(Instance)                                                       \
  _Generic(&*(Instance),                                                       \
  chs             : StringConstructor_Chs,                                     \
  const_chs       : StringConstructor_Chs,                                     \
  wcs             : StringConstructor_Wcs,                                     \
  const_wcs       : StringConstructor_Wcs,                                     \
  unsigned int *  : StringConstructor_Wcs,                                     \
  String_t        : StringConstructor_Str,                                     \
  String          : StringConstructor_Strp)                                    \
  (Instance)
// clang-format on

String
StringConstructor_Chs(const_chs Value);

String
StringConstructor_Wcs(const_wcs Value);

String
StringConstructor_Str(String_t Value);

String
StringConstructor_Strp(String Value);

void
StringModule_Initialized();

extern struct StringMethod StringMethod;

#endif
