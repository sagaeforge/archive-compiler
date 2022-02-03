
#ifndef __STRING__
#define __STRING__

#include <Types/DataTypes_String.h>

// clang-format off

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

String  StringConstructor_Chs     (const_chs pValue);
String  StringConstructor_Wcs     (const_wcs pValue);
String  StringConstructor_Str     (String_t pValue);
String  StringConstructor_Strp    (String pValue);

void    StringModule_Initialized  ();

extern struct StringMethod StringMethod;

#endif
