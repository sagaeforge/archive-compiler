#ifndef __PRIVATE__STRING__
#define __PRIVATE__STRING__

#include <String.h>

// clang-format off
String       String_Join          (String pSelf, String pValue);
void         String_Append        (String pSelf, String pValue);
String       String_SubString     (String pSelf, String pValue);
String       String_Loop          (String pSelf, Length_t pLength);
StringAry    String_Split         (String pSelf, String pValue);
bool         String_Compare       (String pSelf, String pValue);
String       String_Trim          (String pSelf);
bool         String_Contains      (String pSelf, String pValue);
Length_t     String_Count         (String pSelf, String pValue);
wcs          String_Get           (String pSelf);
void         String_Set           (String pSelf, String pValue);
Length_t     String_Length        (String pSelf);
String       String_ToLower       (String pSelf);
String       String_ToUpper       (String pSelf);
bool         String_IsNone        (String pSelf);
Index_t      String_IndexOf       (String pSelf, String pValue);
Index_t      String_IndexAt       (String pSelf, String pValue, Index_t pStart);
Index_t      String_IndexFor      (String pSelf, String pValue, Index_t pIndex);
Index_t      String_LastOfIndex   (String pSelf, String pValue);
String       String_Replace       (String pSelf, String pOri, String pValue);
String       String_ReplaceFor    (String pSelf, String pOri, String pValue, Length_t pLength);
String       String_ReplaceAll    (String pSelf, String pOri, String pValue);
String       String_Left          (String pSelf, Length_t pLength);
String       String_Right         (String pSelf, Length_t pLength);
String       String_Middle        (String pSelf, Index_t pStart, Index_t pEnd);
bool        _StringCompare        (wcs pAry, String pFindValue, Index_t pStart);

#endif