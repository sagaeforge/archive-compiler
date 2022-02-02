#ifndef __PRIVATE__STRING__
#define __PRIVATE__STRING__

#include <String.h>

// clang-format off
String       String_Join(String Self, String Value);
void         String_Append(String Self, String Value);
String       String_SubString(String Self, String Value);
String       String_Loop(String Self, Length_t Length);
StringAry    String_Split(String Self, String Value);
bool         String_Compare(String Self, String Value);
String       String_Trim(String Self);
bool         String_Contains(String Self, String Value);
Length_t     String_Count(String Self, String Value);
wcs          String_Get(String Self);
void         String_Set(String Self, String Value);
Length_t     String_Length(String Self);
String       String_ToLower(String Self);
String       String_ToUpper(String Self);
bool         String_IsNone(String Self);
Index_t      String_IndexOf(String Self, String Value);
Index_t      String_IndexAt(String Self, String Value, Index_t Start);
Index_t      String_IndexFor(String Self, String Value, Index_t Index);
Index_t      String_LastOfIndex(String Self, String Value);
String       String_Replace(String Self, String Ori, String Value);
String       String_ReplaceFor(String Self, String Ori, String Value, Length_t Length);
String       String_ReplaceAll(String Self, String Ori, String Value);
String       String_Left(String Self, Length_t Length);
String       String_Right(String Self, Length_t Length);
String       String_Middle(String Self, Index_t Start, Index_t End);
void         String_Destructor(String* Self);
bool        _StringCompare(wcs Ary, String FindValue, Index_t Start);

#endif