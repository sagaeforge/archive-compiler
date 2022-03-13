#ifndef __NSTRING_H__
#define __NSTRING_H__

// clang-format off

#include <Types/DataType_nString.h>

struct nString_Method {
  nString_ptr       (*Constructor_Chs)   (const Chs_t);
  nString_ptr       (*Constructor_Wcs)   (const Wcs_t);
  nString_ptr       (*Constructor_Str)   (const nString_t);
  nString_ptr       (*Constructor_Strp)  (const nString_ptr);
  bool              (*Destructor)        (      nString_ptr*);
  nString_ptr       (*Join)              (const nString_ptr, const nString_ptr);
  bool              (*Append)            (      nString_ptr, const nString_ptr);
  nString_ptr       (*Loop)              (const nString_ptr, const Length_t);
  bool              (*Compare)           (const nString_ptr, const nString_ptr);
  nString_ptr       (*Trim)              (const nString_ptr);
  Length_t          (*Length)            (const nString_ptr);
  Wcs_t             (*get)               (const nString_ptr);
  bool              (*set)               (      nString_ptr, const nString_ptr);
  nString_ptr       (*Left)              (const nString_ptr, const Length_t);
  nString_ptr       (*Right)             (const nString_ptr, const Length_t);
  nString_ptr       (*Middle)            (const nString_ptr, const Index_t, Index_t);
  nString_ptr       (*Extract)           (const nString_ptr, const Index_t, const Index_t);
  nString_ptr       (*Reverse)           (const nString_ptr);
  nString_ptr       (*ToLower)           (const nString_ptr);
  nString_ptr       (*ToUpper)           (const nString_ptr);
  bool              (*Contains)          (const nString_ptr, const nString_ptr);
  Length_t          (*Count)             (const nString_ptr, const nString_ptr);
  Index_t           (*IndexOf)           (const nString_ptr, const nString_ptr);
  Index_t           (*IndexAt)           (const nString_ptr, const nString_ptr, const Index_t);
  Index_t           (*IndexFor)          (const nString_ptr, const nString_ptr, const Index_t);
  Index_t           (*LastOfIndex)       (const nString_ptr, const nString_ptr);
  nString_ptr       (*Replace)           (const nString_ptr, const nString_ptr, const nString_ptr);
  nString_ptr       (*ReplaceAt)         (const nString_ptr, const nString_ptr, const nString_ptr, const Index_t);
  nString_ptr       (*ReplaceAll)        (const nString_ptr, const nString_ptr, const nString_ptr);
  nStringAry_ptr    (*Split)             (const nString_ptr, const nString_ptr);
  nString_ptr       (*SubString)         (const nString_ptr, const nString_ptr);
  bool              (*Pattern)           (const nString_ptr, const nString_ptr);
  bool              (*isAlpha)           (const nString_ptr);
  bool              (*isLower)           (const nString_ptr);
  bool              (*isUpper)           (const nString_ptr);
  bool              (*isDecimal)         (const nString_ptr);
  bool              (*isDigit)           (const nString_ptr);
  bool              (*isSpace)           (const nString_ptr);
  bool              (*isAlphaDigit)      (const nString_ptr);
  bool              (*isHex)             (const nString_ptr);
  bool              (*isControl)         (const nString_ptr);
  bool              (*isOctal)           (const nString_ptr);
  bool              (*isBinary)          (const nString_ptr);
  bool              (*Search)            (const nString_ptr, const nStringAry_ptr);
  bool              (*Check)             (const wchar_t, const nString_ptr);
  nString_ptr       (*Format)            (const nString_ptr, ...);
  nString_ptr       (*Notation)          (const int64_t, const int);
  nStringAry_ptr    (*FileAllRead)       (FILE*);
  bool              (*FileAllWrite)      (const nStringAry_ptr, FILE*);
  Wcs_t             (*UTF8Decorder)      (Chs_t, Length_t*);
  Chs_t             (*UTF8Encoder)       (Wcs_t, Length_t*);
};
struct nStringAry_Method {
  nStringAry_ptr    (*Constructor)       (const Length_t, ...);
  bool              (*Destructor)        (      nStringAry_ptr*);
  bool              (*Clear)             (      nStringAry_ptr);
  nString_ptr       (*get)               (const nStringAry_ptr, const Index_t);
  bool              (*set)               (const nStringAry_ptr, const Index_t, const nString_ptr);
  bool              (*Insert)            (      nStringAry_ptr, const nString_ptr, Index_t);
  bool              (*Remove)            (      nStringAry_ptr, const Index_t);
  bool              (*Push)              (      nStringAry_ptr, const nString_ptr);
  nString_ptr       (*Pop)               (      nStringAry_ptr);
  Index_t           (*Search)            (const nStringAry_ptr, const nString_ptr);
  bool              (*Contains)          (const nStringAry_ptr, const nString_ptr);
  nStringAry_ptr    (*toAry)             (const nStringAry_ptr);
  nStringAry_ptr    (*toList)            (const nStringAry_ptr);
  nStringAryType_t  (*Type)              (const nStringAry_ptr);
  Length_t          (*CountIf)           (const nStringAry_ptr, bool (*)(nString_ptr, void *), void *);
  Length_t          (*Length)            (const nStringAry_ptr);
};
struct nRegExp_Method {
  nRegExp_ptr       (*Constructor_Str)   (nString_ptr, nString_ptr);
  nRegExp_ptr       (*Constructor_Clone) (const nRegExp_ptr);
  bool              (*Destructor)        (      nRegExp_ptr*);
  nRegExpResult_ptr (*Analysis)          (const nString_ptr, const nRegExp_ptr);
  Length_t          (*Count)             (const nString_ptr, const nRegExp_ptr);
  Index_t           (*IndexOf)           (const nString_ptr, const nRegExp_ptr);
  Index_t           (*IndexAt)           (const nString_ptr, const nRegExp_ptr, const Index_t);
  Index_t           (*IndexFor)          (const nString_ptr, const nRegExp_ptr, const Index_t);
  Index_t*          (*IndexAll)          (const nString_ptr, const nRegExp_ptr);
  Index_t           (*LastOfIndex)       (const nString_ptr, const nRegExp_ptr);
  nString_ptr       (*Replace)           (const nString_ptr, const nRegExp_ptr, const nString_ptr);
  nString_ptr       (*ReplaceAt)         (const nString_ptr, const nRegExp_ptr, const nString_ptr, const Index_t);
  nString_ptr       (*ReplaceAll)        (const nString_ptr, const nRegExp_ptr, const nString_ptr);
  nStringAry_ptr    (*Split)             (const nString_ptr, const nRegExp_ptr);
  nString_ptr       (*SubString)         (const nString_ptr, const nRegExp_ptr);
};
struct nChs_Method {
  Length_t          (*__ChsLen)          (const Chs_t);
  Length_t          (*__WcsLen)          (const Wcs_t);
  bool              (*__IsAlpha)         (const int);
  bool              (*__IsLower)         (const int);
  bool              (*__IsUpper)         (const int);
  bool              (*__IsDecimal)       (const int);
  bool              (*__IsDigit)         (const int);
  bool              (*__IsSpace)         (const int);
  bool              (*__IsAlphaDigit)    (const int);
  bool              (*__IsHex)           (const int);
  bool              (*__IsControl)       (const int);
  bool              (*__IsOctal)         (const int);
  bool              (*__IsBinary)        (const int);
  int               (*__ToLower)         (const int);
  int               (*__ToUpper)         (const int);
};

extern struct nString_Method StringMethod[];
extern struct nStringAry_Method StringAryMethod[];
extern struct nRegExp_Method RegExpMethod[];
extern struct nChs_Method ChsMethod[];

#pragma region 함수 선언 

#define nString(Instance) _Generic((Instance),                \
  Chs_t             : nString_Constructor_Chs,                \
  const Chs_t       : nString_Constructor_Chs,                \
  Wcs_t             : nString_Constructor_Wcs,                \
  const Wcs_t       : nString_Constructor_Wcs,                \
  nString_t         : nString_Constructor_Str,                \
  nString_ptr       : nString_Constructor_Strp,               \
  const nString_ptr : nString_Constructor_Strp)               \
  (Instance)

nString_ptr        nString_Constructor_Chs            (const Chs_t pValue);
nString_ptr        nString_Constructor_Wcs            (const Wcs_t pValue);
nString_ptr        nString_Constructor_Str            (const nString_t   pValue);
nString_ptr        nString_Constructor_Strp           (const nString_ptr  pValue);
bool               nString_Destructor                 (      nString_ptr* pSelf);
nString_ptr        nString_Join                       (const nString_ptr  pSelf, const nString_ptr pValue);
bool               nString_Append                     (      nString_ptr  pSelf, const nString_ptr pValue);
nString_ptr        nString_Loop                       (const nString_ptr  pSelf, const Length_t pLength);
bool               nString_Compare                    (const nString_ptr  pSelf, const nString_ptr pValue);
nString_ptr        nString_Trim                       (const nString_ptr  pSelf);
Length_t           nString_Length                     (const nString_ptr  pSelf);
Wcs_t              nString_get                        (const nString_ptr  pSelf);
bool               nString_set                        (      nString_ptr  pSelf, const nString_ptr pValue);
nString_ptr        nString_Left                       (const nString_ptr  pSelf, const Length_t pLength);
nString_ptr        nString_Right                      (const nString_ptr  pSelf, const Length_t pLength);
nString_ptr        nString_Middle                     (const nString_ptr  pSelf, const Index_t pStart, Index_t pEnd);
nString_ptr        nString_Extract                    (const nString_ptr  pSelf, const Index_t pStart, const Index_t pEnd);
nString_ptr        nString_Reverse                    (const nString_ptr  pSelf);
nString_ptr        nString_ToLower                    (const nString_ptr  pSelf);
nString_ptr        nString_ToUpper                    (const nString_ptr  pSelf);
bool               nString_Contains                   (const nString_ptr  pSelf, const nString_ptr pKeyWord);
Length_t           nString_Count                      (const nString_ptr  pSelf, const nString_ptr pKeyWord);
Index_t            nString_IndexOf                    (const nString_ptr  pSelf, const nString_ptr pKeyWord);
Index_t            nString_IndexAt                    (const nString_ptr  pSelf, const nString_ptr pKeyWord, const Index_t pIndex);
Index_t            nString_IndexFor                   (const nString_ptr  pSelf, const nString_ptr pKeyWord, const Index_t pStart);
Index_t            nString_LastOfIndex                (const nString_ptr  pSelf, const nString_ptr pKeyWord);
nString_ptr        nString_Replace                    (const nString_ptr  pSelf, const nString_ptr pKeyWord, const nString_ptr pValue);
nString_ptr        nString_ReplaceAt                  (const nString_ptr  pSelf, const nString_ptr pKeyWord, const nString_ptr pValue, const Index_t pIndex);
nString_ptr        nString_ReplaceAll                 (const nString_ptr  pSelf, const nString_ptr pKeyWord, const nString_ptr pValue);
nStringAry_ptr     nString_Split                      (const nString_ptr  pSelf, const nString_ptr pKeyWord);
nString_ptr        nString_SubString                  (const nString_ptr  pSelf, const nString_ptr pKeyWord);
bool               nString_Pattern                    (const nString_ptr  pSelf, const nString_ptr pKeyWord);
bool               nString_isAlpha                    (const nString_ptr  pSelf);
bool               nString_isLower                    (const nString_ptr  pSelf);
bool               nString_isUpper                    (const nString_ptr  pSelf);
bool               nString_isDecimal                  (const nString_ptr  pSelf);
bool               nString_isDigit                    (const nString_ptr  pSelf);
bool               nString_isSpace                    (const nString_ptr  pSelf);
bool               nString_isAlphaDigit               (const nString_ptr  pSelf);
bool               nString_isHex                      (const nString_ptr  pSelf);
bool               nString_isControl                  (const nString_ptr  pSelf);
bool               nString_isOctal                    (const nString_ptr  pSelf);
bool               nString_isBinary                   (const nString_ptr  pSelf);
bool               nString_Check                      (const wchar_t pChar, const nString_ptr pFindAry);
nString_ptr        nString_Format                     (const nString_ptr  pFormat, ...);
nString_ptr        nString_Notation                   (const int64_t pValue, const int pNotation);
nStringAry_ptr     nString_FileAllRead                (FILE* pFile);
bool               nString_FileAllWrite               (const nStringAry_ptr pSelf, FILE* pFile);
Wcs_t              nString_UTF8Decoder                (Chs_t pValue, Length_t* out_pValueSize);
Chs_t              nString_UTF8Encoder                (Wcs_t pValue, Length_t* out_pValueSize);

#define toString(DataType) ((nString_ptr(*)(DataType)) nString_toStringSearch(#DataType))
Func_t nString_toStringSearch(const char* DataType);

#define ValueOf(DataType) ((DataType(*)(nString_ptr)) nString_ValueOfSearch(#DataType))
Func_t nString_ValueOfSearch(const char* DataType);

#define nStringAry(count, args...) nStringAry_Constructor(count, ##args)
nStringAry_ptr     nStringAry_Constructor             (const Length_t pCount, ...);
bool               nStringAry_Destructor              (      nStringAry_ptr* pSelf);
bool               nStringAry_Clear                   (      nStringAry_ptr  pSelf);
nString_ptr        nStringAry_get                     (const nStringAry_ptr  pSelf, const Index_t pIndex);
bool               nStringAry_set                     (const nStringAry_ptr  pSelf, const Index_t pIndex, const nString_ptr pValue);
bool               nStringAry_Insert                  (      nStringAry_ptr  pSelf, const nString_ptr pValue, Index_t pIndex);
bool               nStringAry_Remove                  (      nStringAry_ptr  pSelf, const Index_t pIndex);
bool               nStringAry_Push                    (      nStringAry_ptr  pSelf, const nString_ptr pValue);
nString_ptr        nStringAry_Pop                     (      nStringAry_ptr  pSelf);
Index_t            nStringAry_Search                  (const nStringAry_ptr  pSelf, const nString_ptr pValue);
bool               nStringAry_Contains                (const nStringAry_ptr  pSelf, const nString_ptr pValue);
nStringAry_ptr     nStringAry_toAry                   (const nStringAry_ptr  pSelf);
nStringAry_ptr     nStringAry_toList                  (const nStringAry_ptr  pSelf);
nStringAryType_t   nStringAry_Type                    (const nStringAry_ptr  pSelf);
Length_t           nStringAry_CountIf                 (const nStringAry_ptr  pSelf, bool (*pExpression)(nString_ptr, void *), void *pParam);
Length_t           nStringAry_Length                  (const nStringAry_ptr  pSelf);

#define strLen(Instance) _Generic((Instance),                 \
  Chs_t               : __ChsLen,                             \
  const Chs_t         : __ChsLen,                             \
  Wcs_t               : __WcsLen,                             \
  const Wcs_t         : __WcsLen,                             \
  nString_ptr         : nString_Length,                       \
  const nString_ptr   : nString_Length,                       \
  nStringAry_t        : nStringAry_Length,                    \
  const nStringAry_t  : nStringAry_Length                     \
  ) (Instance)

#define nRegExp(RegExp, args...) _Generic((Instance),         \
  nString_t         : nRegExp_Constructor_Str,                \
  nString_ptr       : nRegExp_Constructor_Str,                \
  const nString_ptr : nRegExp_Constructor_Str,                \
  nRegExp_t         : nRegExp_Constructor_Clone,              \
  nRegExp_ptr       : nRegExp_Constructor_Clone,              \
  const nRegExp_ptr : nRegExp_Constructor_Clone)              \
  (RegExp, ##args)

nRegExp_ptr        nRegExp_Constructor_Str            (nString_ptr pRegExp, nString_ptr pFlag);
nRegExp_ptr        nRegExp_Constructor_Clone          (const nRegExp_ptr  pSelf);
bool               nRegExp_Destructor                 (      nRegExp_ptr* pSelf);
nRegExpResult_ptr  nRegExp_Analysis                   (const nString_ptr  pSelf, const nRegExp_ptr pRegExp);
Length_t           nRegExp_Count                      (const nString_ptr  pSelf, const nRegExp_ptr pRegExp);
Index_t            nRegExp_IndexOf                    (const nString_ptr  pSelf, const nRegExp_ptr pRegExp);
Index_t            nRegExp_IndexAt                    (const nString_ptr  pSelf, const nRegExp_ptr pRegExp, const Index_t pIndex);
Index_t            nRegExp_IndexFor                   (const nString_ptr  pSelf, const nRegExp_ptr pRegExp, const Index_t pStart);
Index_t*           nRegExp_IndexAll                   (const nString_ptr  pSelf, const nRegExp_ptr pRegExp);
Index_t            nRegExp_LastOfIndex                (const nString_ptr  pSelf, const nRegExp_ptr pRegExp);
nString_ptr        nRegExp_Replace                    (const nString_ptr  pSelf, const nRegExp_ptr pRegExp, const nString_ptr pValue);
nString_ptr        nRegExp_ReplaceAt                  (const nString_ptr  pSelf, const nRegExp_ptr pRegExp, const nString_ptr pValue, const Index_t pIndex);
nString_ptr        nRegExp_ReplaceAll                 (const nString_ptr  pSelf, const nRegExp_ptr pRegExp, const nString_ptr pValue);
nStringAry_ptr     nRegExp_Split                      (const nString_ptr  pSelf, const nRegExp_ptr pRegExp);
nString_ptr        nRegExp_SubString                  (const nString_ptr  pSelf, const nRegExp_ptr pRegExp);

Length_t           nChs_ChsLen                        (const Chs_t pValue);
Length_t           nChs_WcsLen                        (const Wcs_t pValue);
bool               nChs_IsAlpha                       (const int pCh);
bool               nChs_IsLower                       (const int pCh);
bool               nChs_IsUpper                       (const int pCh);
bool               nChs_IsDecimal                     (const int pCh);
bool               nChs_IsDigit                       (const int pCh);
bool               nChs_IsSpace                       (const int pCh);
bool               nChs_IsAlphaDigit                  (const int pCh);
bool               nChs_IsHex                         (const int pCh);
bool               nChs_IsControl                     (const int pCh);
bool               nChs_IsOctal                       (const int pCh);
bool               nChs_IsBinary                      (const int pCh);
int                nChs_ToLower                       (const int pCh);
int                nChs_ToUpper                       (const int pCh);

#pragma endregion
#endif // __NSTRING_H__