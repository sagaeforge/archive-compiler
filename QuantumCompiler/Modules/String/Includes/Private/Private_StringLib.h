#ifndef __PRIVATE_STRING_LIBRARY__
#define __PRIVATE_STRING_LIBRARY__

#include "StringLib.h"

String*
String_Format(String* Format, ...);
bool
String_Pattern(String* Self, String* Format);
String*
String_Extract(String* Self, Index Start, Index End);
String*
String_Notation(_int64 Value, int notation);
String*
String_Reverse(String* Self);
String*
String_Prettier(double Value);

bool
String_IsAlpha(String* Self);
bool
String_IsLower(String* Self);
bool
String_IsUpper(String* Self);
bool
String_IsDecimal(String* Self);
bool
String_IsDigit(String* Self);
bool
String_IsSpace(String* Self);
bool
String_IsAlphaDigit(String* Self);
bool
String_IsHex(String* Self);
bool
String_IsControl(String* Self);

bool
String_IsOctal(String* Self);
bool
String_IsBinary(String* Self);

#endif