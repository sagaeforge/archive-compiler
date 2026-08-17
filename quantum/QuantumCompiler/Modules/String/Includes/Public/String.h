
#ifndef __STRING__
#define __STRING__

#include <Types/DataType_Object.h>
#include <Types/DataTypes_String.h>

// clang-format off

#define String(Instance)                                                       \
  _Generic(&*(Instance),                                                       \
  chs             : String_Constructor_Chs,                                     \
  const_chs       : String_Constructor_Chs,                                     \
  wcs             : String_Constructor_Wcs,                                     \
  const_wcs       : String_Constructor_Wcs,                                     \
  unsigned int *  : String_Constructor_Wcs,                                     \
  String_t        : String_Constructor_Str,                                     \
  String          : String_Constructor_Strp)                                    \
  (Instance)

String  String_Constructor_Chs     (const_chs pValue);
String  String_Constructor_Wcs     (const_wcs pValue);
String  String_Constructor_Str     (String_t pValue);
String  String_Constructor_Strp    (String pValue);
void    String_Destructor          (String* pSelf);

__attribute__((warn_unused_result)) const Object          __Object_Boxing_String            (const String     pValue);
__attribute__((warn_unused_result)) const Object          __Object_Boxing_StringAry         (const StringAry  pValue);
                                    String                __Object_UnBoxing_String          (const Object     pSelf);
                                    StringAry             __Object_UnBoxing_StringAry       (const Object     pSelf);

void    StringModule_Initialized  ();

extern struct StringMethod StringMethod;

#endif
