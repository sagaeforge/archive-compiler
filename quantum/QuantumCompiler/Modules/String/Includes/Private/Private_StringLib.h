#ifndef __PRIVATE_STRING_LIBRARY__
#define __PRIVATE_STRING_LIBRARY__

#include <StringLib.h>

// clang-format off

String String_Format          (String pFormat, ...);
bool String_Pattern           (String pSelf, String pFormat);
String String_Extract         (String pSelf, Index_t pStart, Index_t pEnd);
String String_Notation        (int64_t pValue, int pNotation);
String String_Reverse         (String pSelf);
String String_Prettier        (double pValue);
StringAry String_FileAllRead  (FILE* pfile);
bool String_FileAllWrite      (StringAry pSelf, FILE* pFile);

bool String_IsAlpha           (String pSelf);
bool String_IsLower           (String pSelf);
bool String_IsUpper           (String pSelf);
bool String_IsDecimal         (String pSelf);
bool String_IsDigit           (String pSelf);
bool String_IsSpace           (String pSelf);
bool String_IsAlphaDigit      (String pSelf);
bool String_IsHex             (String pSelf);
bool String_IsControl         (String pSelf);
bool String_IsOctal           (String pSelf);
bool String_IsBinary          (String pSelf);

#endif