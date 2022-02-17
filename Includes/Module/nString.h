#ifndef __NSTRING_H__
#define __NSTRING_H__

#include <stdio.h>

#include <Module/Types/nString.h>

// clang-format off

#define nString(Instance) _Generic((Instance),                                    \
  Chs_t             : String_Constructor_Chs,                                     \
  const Chs_t       : String_Constructor_Chs,                                     \
  Wcs_t             : String_Constructor_Wcs,                                     \
  const Wcs_t       : String_Constructor_Wcs,                                     \
  nString_t         : String_Constructor_Str,                                     \
  nString_t*        : String_Constructor_Strp,                                    \
  const nString_t*  : String_Constructor_Strp)                                    \
  (Instance)

nString_t*        String_Constructor_Chs            (const Chs_t pValue);
nString_t*        String_Constructor_Wcs            (const Wcs_t pValue);
nString_t*        String_Constructor_Str            (const nString_t   pValue);
nString_t*        String_Constructor_Strp           (const nString_t*  pValue);
bool              String_Destructor                 (      nString_t** pSelf);
nString_t*        String_Join                       (const nString_t*  pSelf, const nString_t* pValue);
bool              String_Append                     (      nString_t*  pSelf, const nString_t* pValue);
nString_t*        String_SubString                  (const nString_t*  pSelf, const nString_t* pValue);
nString_t*        String_Loop                       (const nString_t*  pSelf, const Length_t pLength);
nStringAry_t*     String_Split                      (const nString_t*  pSelf, const nString_t* pValue);
bool              String_Compare                    (const nString_t*  pSelf, const nString_t* pValue);
nString_t*        String_Trim                       (const nString_t*  pSelf);
bool              String_Contains                   (const nString_t*  pSelf, const nString_t* pValue);
Length_t          String_Count                      (const nString_t*  pSelf, const nString_t* pValue);
Wcs_t             String_Get                        (const nString_t*  pSelf);
bool              String_Set                        (      nString_t*  pSelf, const nString_t* pValue);
Length_t          String_Length                     (const nString_t*  pSelf);
nString_t*        String_ToLower                    (const nString_t*  pSelf);
nString_t*        String_ToUpper                    (const nString_t*  pSelf);
Index_t           String_IndexOf                    (const nString_t*  pSelf, const nString_t* pValue);
Index_t           String_IndexAt                    (const nString_t*  pSelf, const nString_t* pValue, const Index_t pIndex);
Index_t           String_IndexFor                   (const nString_t*  pSelf, const nString_t* pValue, const Index_t pStart);
Index_t           String_LastOfIndex                (const nString_t*  pSelf, const nString_t* pValue);
nString_t*        String_Replace                    (const nString_t*  pSelf, const nString_t* pOri, const nString_t* pValue);
nString_t*        String_ReplaceAt                  (const nString_t*  pSelf, const nString_t* pOri, const nString_t* pValue, const Index_t pIndex);
nString_t*        String_ReplaceAll                 (const nString_t*  pSelf, const nString_t* pOri, const nString_t* pValue);
nString_t*        String_Left                       (const nString_t*  pSelf, const Length_t pLength);
nString_t*        String_Right                      (const nString_t*  pSelf, const Length_t pLength);
nString_t*        String_Middle                     (const nString_t*  pSelf, const Index_t pStart, Index_t pEnd);
nString_t*        String_Extract                    (const nString_t*  pSelf, const Index_t pStart, const Index_t pEnd);
nString_t*        String_Reverse                    (const nString_t*  pSelf);
bool              String_Search                     (const nString_t*  pSelf, const nStringAry_t* pFindAry); // pSelf가 pFindAry중에 하나라면
bool              String_IsAlpha                    (const nString_t*  pSelf);
bool              String_IsLower                    (const nString_t*  pSelf);
bool              String_IsUpper                    (const nString_t*  pSelf);
bool              String_IsDecimal                  (const nString_t*  pSelf);
bool              String_IsDigit                    (const nString_t*  pSelf);
bool              String_IsSpace                    (const nString_t*  pSelf);
bool              String_IsAlphaDigit               (const nString_t*  pSelf);
bool              String_IsHex                      (const nString_t*  pSelf);
bool              String_IsControl                  (const nString_t*  pSelf);
bool              String_IsOctal                    (const nString_t*  pSelf);
bool              String_IsBinary                   (const nString_t*  pSelf);
nString_t*        String_Format                     (const nString_t*  pFormat, ...);
bool              String_Pattern                    (const nString_t*  pSelf, const nString_t* pFormat);
bool              String_Check                      (const wchar_t pChar, const nString_t* pFindAry);    // pChar가 pFindAry중에 하나라면
nString_t*        String_Notation                   (const int64_t pValue, const int pNotation);
nStringAry_t*     String_FileAllRead                (FILE* pFile);
bool              String_FileAllWrite               (const nStringAry_t* pSelf, FILE* pFile);
nString_t*        String_Print                      (const nString_t* pFormat, ...);
nString_t*        String_PrintErr                   (const nString_t* pFormat, ...);
nString_t*        String_PrintLine                  (const nString_t* pFormat, ...);

#define toString(Instance, args...) _Generic((Instance),         \
  _Bool               : String_ToString_Bool,                    \
  int                 : String_ToString_Decimal,                 \
  unsigned int        : String_ToString_Decimal_Unsigned,        \
  long                : String_ToString_Decimal,                 \
  unsigned long       : String_ToString_Decimal_Unsigned,        \
  long long           : String_ToString_Decimal,                 \
  unsigned long long  : String_ToString_Decimal_Unsigned,        \
  float               : String_ToString_Digit,                   \
  double              : String_ToString_Digit,                   \
  const nStringAry_t* : String_ToString_StringAry,               \
  nStringAry_t*       : String_ToString_StringAry)               \
  (Instance, ##args)

#define ValueOf(DataType) ((DataType(*)(String)) String_ValueOfSearch(#DataType))

Func_t String_ValueOfSearch(const char* DataType);

nString_t*        String_ToString_Bool              (const bool pValue);
nString_t*        String_ToString_Decimal           (const int64_t pValue);
nString_t*        String_ToString_Decimal_Unsigned  (const uint64_t pValue);
nString_t*        String_ToString_Digit             (const double pValue, const int numDigit);
nString_t*        String_ToString_StringAry         (const nStringAry_t* pValue, const nString_t* pReplaceWord);

bool              String_ValueOf_Bool               (const nString_t* pSelf);
int64_t           String_ValueOf_Decimal            (const nString_t* pSelf);
uint64_t          String_ValueOf_Decimal_Unsigned   (const nString_t* pSelf);
double            String_ValueOf_Digit              (const nString_t* pSelf);   

Wcs_t             String_UTF8Decorder               (Chs_t pValue, Length_t* out_pValueSize);
Chs_t             String_UTF8Encoder                (Wcs_t pValue, Length_t* out_pValueSize);

#define STR(_STR_) L#_STR_

#define __WCSMAKE(Len) (Wcs_t)calloc(sizeof(wchar_t), ((Len) + 1))
#define __WCSBUFMAKE(Len) Wcs_t _BUF = __WCSMAKE(Len)
#define __FLUSHWCSBUF() free(_BUF)

#define __STRLEN(Instance) _Generic((Instance),            \
  Chs_t       : __ChsLen,                                  \
  const Chs_t : __ChsLen,                                  \
  Wcs_t       : __WcsLen,                                  \
  const Wcs_t : __WcsLen                                   \
)(Instance)
Length_t __ChsLen(const Chs_t pValue);
Length_t __WcsLen(const Wcs_t pValue);

bool              __IsAlpha                         (const int pCh);
bool              __IsLower                         (const int pCh);
bool              __IsUpper                         (const int pCh);
bool              __IsDecimal                       (const int pCh);
bool              __IsDigit                         (const int pCh);
bool              __IsSpace                         (const int pCh);
bool              __IsAlphaDigit                    (const int pCh);
bool              __IsHex                           (const int pCh);
bool              __IsControl                       (const int pCh);
bool              __IsOctal                         (const int pCh);
bool              __IsBinary                        (const int pCh);
int               __ToLower                         (const int pCh);
int               __ToUpper                         (const int pCh);

#endif // __NSTRING_H__