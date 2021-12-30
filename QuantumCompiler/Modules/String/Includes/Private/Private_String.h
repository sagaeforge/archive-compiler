#ifndef __PRIVATE__STRING__
#define __PRIVATE__STRING__

#include "String.h"

// clang-format off
String    *String_Join        (String *Self, String *Value);
void       String_Append      (String *Self, String *Value);
String    *String_SubString   (String *Self, String *Value);
String    *String_Loop        (String *Self, Length Length);
StringAry *String_Split       (String *Self, String *Value);
bool       String_Compare     (String *Self, String *Value);
String    *String_Trim        (String *Self);
bool       String_Contains    (String *Self, String *Value);
Length     String_Count       (String *Self, String *Value);
wcs        String_Get         (String *Self);
void       String_Set         (String *Self, String *Value);
Length     String_Length      (String *Self);
String    *String_ToLower     (String *Self);
String    *String_ToUpper     (String *Self);
bool       String_IsNone      (String *Self);
int        String_IndexOf     (String *Self, String *Value);
int        String_LastOfIndex (String *Self, String *Value);
String    *String_Replace     (String *Self, String *Ori, String *Value);
String    *String_ReplaceFor  (String *Self, String *Ori, String *Value, Length Length);
String    *String_ReplaceAll  (String *Self, String *Ori, String *Value);
String    *String_Left        (String *Self, Length Length);
String    *String_Right       (String *Self, Length Length);
String    *String_Middle      (String *Self, Index Start, Index End);
void       String_Const       (String *Self);
void       String_UnConst     (String *Self);
void       String_Destructor  (String **Self);
// clang-format on

#endif