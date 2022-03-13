#ifndef __NSTRING_H__
#define __NSTRING_H__

// clang-format off

#pragma region 자료 선언
#pragma pack(push, 1)

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <wchar.h>

#ifndef __DEFAULT_DATATYPES
typedef uint32_t Length_t;
typedef uint32_t Index_t;
typedef void (*Func_t)(void);
#endif // !__DEFAULT_DATATYPES

typedef char *Chs_t;
typedef wchar_t *Wcs_t;

typedef struct {
  Wcs_t m_Value;
  Length_t m_Length;
} nString_t,
 *nString_ptr;

typedef struct nStringAryNode {
  nString_ptr m_Value;
  struct nStringAryNode *Next;
} nStringAryNode_t, 
 *nStringAryNode_ptr;

typedef enum {
  k_nStringAry_None = 0,
  k_nStringAry_Ary = 1,
  k_nStringAry_List = 2,
} nStringAryType_t,
  nStringAryType_enum;

typedef struct {
  Length_t m_Size;
  Length_t m_Length;
  nStringAryType_t m_AryType;
  union {
    nString_ptr *m_Arys;
    nStringAryNode_ptr m_Lists;
  };
} nStringAry_t,
 *nStringAry_ptr;

typedef enum {
  k_nRegExpFlag_None        = 0,
  k_nRegExpFlag_Global      = (1 << 0),
  k_nRegExpFlag_HasIndices  = (1 << 1),
  k_nRegExpFlag_IgnoreCase  = (1 << 2),
  k_nRegExpFlag_Multiline   = (1 << 3),
  k_nRegExpFlag_DotAll      = (1 << 4),
  k_nRegExpFlag_Unicode     = (1 << 5),
  k_nRegExpFlag_Sticky      = (1 << 6)
} nRegExpFlag_t,
  nRegExpFlag_enum;

typedef struct {
  nRegExpFlag_t m_Flag;
  nString_ptr m_Pattern;
} nRegExp_t,
 *nRegExp_ptr;

typedef struct nRegExpResultNode {
  Index_t StartIndex;
  Index_t LastIndex;
  struct nRegExpResultNode *Next;
} nRegExpResultNode_t,
 *nRegExpResultNode_ptr;

typedef struct {
  nString_ptr m_OrignalText;
  nRegExp_ptr m_RegExp;
  bool isFound;
  struct {
    Length_t m_Count;
    nRegExpResultNode_ptr m_Nodes;
  } m_Result;
} nRegExpResult_t,
 *nRegExpResult_ptr;

#pragma pack(pop)

#pragma endregion
#pragma region 전역 함수 집합
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
#pragma endregion
#pragma region 함수 선언

#define nString(Instance) _Generic((Instance),                                    \
  Chs_t             : String_Constructor_Chs,                                     \
  const Chs_t       : String_Constructor_Chs,                                     \
  Wcs_t             : String_Constructor_Wcs,                                     \
  const Wcs_t       : String_Constructor_Wcs,                                     \
  nString_t         : String_Constructor_Str,                                     \
  nString_ptr       : String_Constructor_Strp,                                    \
  const nString_ptr : String_Constructor_Strp)                                    \
  (Instance)

nString_ptr        String_Constructor_Chs            (const Chs_t pValue);
nString_ptr        String_Constructor_Wcs            (const Wcs_t pValue);
nString_ptr        String_Constructor_Str            (const nString_t   pValue);
nString_ptr        String_Constructor_Strp           (const nString_ptr  pValue);
bool               String_Destructor                 (      nString_ptr* pSelf);
nString_ptr        String_Join                       (const nString_ptr  pSelf, const nString_ptr pValue);
bool               String_Append                     (      nString_ptr  pSelf, const nString_ptr pValue);
nString_ptr        String_Loop                       (const nString_ptr  pSelf, const Length_t pLength);
bool               String_Compare                    (const nString_ptr  pSelf, const nString_ptr pValue);
nString_ptr        String_Trim                       (const nString_ptr  pSelf);
Length_t           String_Length                     (const nString_ptr  pSelf);
Wcs_t              String_get                        (const nString_ptr  pSelf);
bool               String_set                        (      nString_ptr  pSelf, const nString_ptr pValue);
nString_ptr        String_Left                       (const nString_ptr  pSelf, const Length_t pLength);
nString_ptr        String_Right                      (const nString_ptr  pSelf, const Length_t pLength);
nString_ptr        String_Middle                     (const nString_ptr  pSelf, const Index_t pStart, Index_t pEnd);
nString_ptr        String_Extract                    (const nString_ptr  pSelf, const Index_t pStart, const Index_t pEnd);
nString_ptr        String_Reverse                    (const nString_ptr  pSelf);
nString_ptr        String_ToLower                    (const nString_ptr  pSelf);
nString_ptr        String_ToUpper                    (const nString_ptr  pSelf);
bool               String_Contains                   (const nString_ptr  pSelf, const nString_ptr pKeyWord);
Length_t           String_Count                      (const nString_ptr  pSelf, const nString_ptr pKeyWord);
Index_t            String_IndexOf                    (const nString_ptr  pSelf, const nString_ptr pKeyWord);
Index_t            String_IndexAt                    (const nString_ptr  pSelf, const nString_ptr pKeyWord, const Index_t pIndex);
Index_t            String_IndexFor                   (const nString_ptr  pSelf, const nString_ptr pKeyWord, const Index_t pStart);
Index_t            String_LastOfIndex                (const nString_ptr  pSelf, const nString_ptr pKeyWord);
nString_ptr        String_Replace                    (const nString_ptr  pSelf, const nString_ptr pKeyWord, const nString_ptr pValue);
nString_ptr        String_ReplaceAt                  (const nString_ptr  pSelf, const nString_ptr pKeyWord, const nString_ptr pValue, const Index_t pIndex);
nString_ptr        String_ReplaceAll                 (const nString_ptr  pSelf, const nString_ptr pKeyWord, const nString_ptr pValue);
nStringAry_ptr     String_Split                      (const nString_ptr  pSelf, const nString_ptr pKeyWord);
nString_ptr        String_SubString                  (const nString_ptr  pSelf, const nString_ptr pKeyWord);
bool               String_Pattern                    (const nString_ptr  pSelf, const nString_ptr pKeyWord);
bool               String_isAlpha                    (const nString_ptr  pSelf);
bool               String_isLower                    (const nString_ptr  pSelf);
bool               String_isUpper                    (const nString_ptr  pSelf);
bool               String_isDecimal                  (const nString_ptr  pSelf);
bool               String_isDigit                    (const nString_ptr  pSelf);
bool               String_isSpace                    (const nString_ptr  pSelf);
bool               String_isAlphaDigit               (const nString_ptr  pSelf);
bool               String_isHex                      (const nString_ptr  pSelf);
bool               String_isControl                  (const nString_ptr  pSelf);
bool               String_isOctal                    (const nString_ptr  pSelf);
bool               String_isBinary                   (const nString_ptr  pSelf);
bool               String_Search                     (const nString_ptr  pSelf, const nStringAry_ptr pFindAry); // pSelf가 pFindAry중에 하나라면
bool               String_Check                      (const wchar_t pChar, const nString_ptr pFindAry);    // pChar가 pFindAry중에 하나라면
nString_ptr        String_Format                     (const nString_ptr  pFormat, ...);
nString_ptr        String_Notation                   (const int64_t pValue, const int pNotation);
nStringAry_ptr     String_FileAllRead                (FILE* pFile);
bool               String_FileAllWrite               (const nStringAry_ptr pSelf, FILE* pFile);

#define toString(DataType) ((nString_ptr(*)(DataType)) String_toStringSearch(#DataType))
Func_t String_toStringSearch(const char* DataType);

#define ValueOf(DataType) ((DataType(*)(nString_ptr)) String_ValueOfSearch(#DataType))
Func_t String_ValueOfSearch(const char* DataType);

#define nStringAry(count, args...) StringAry_Constructor(count, ##args)
nStringAry_ptr     StringAry_Constructor             (const Length_t pCount, ...);
bool               StringAry_Destructor              (      nStringAry_ptr* pSelf);
bool               StringAry_Clear                   (      nStringAry_ptr  pSelf);
nString_ptr        StringAry_get                     (const nStringAry_ptr  pSelf, const Index_t pIndex);
bool               StringAry_set                     (const nStringAry_ptr  pSelf, const Index_t pIndex, const nString_ptr pValue);
bool               StringAry_Insert                  (      nStringAry_ptr  pSelf, const nString_ptr pValue, Index_t pIndex);
bool               StringAry_Remove                  (      nStringAry_ptr  pSelf, const Index_t pIndex);
bool               StringAry_Push                    (      nStringAry_ptr  pSelf, const nString_ptr pValue);
nString_ptr        StringAry_Pop                     (      nStringAry_ptr  pSelf);
Index_t            StringAry_Search                  (const nStringAry_ptr  pSelf, const nString_ptr pValue);
bool               StringAry_Contains                (const nStringAry_ptr  pSelf, const nString_ptr pValue);
nStringAry_ptr     StringAry_toAry                   (const nStringAry_ptr  pSelf);
nStringAry_ptr     StringAry_toList                  (const nStringAry_ptr  pSelf);
nStringAryType_t   StringAry_Type                    (const nStringAry_ptr  pSelf);
Length_t           StringAry_CountIf                 (const nStringAry_ptr  pSelf, bool (*pExpression)(nString_ptr, void *), void *pParam);
Length_t           StringAry_Length                  (const nStringAry_ptr  pSelf);

#define strLen(Instance) _Generic((Instance),                 \
  Chs_t               : __ChsLen,                             \
  const Chs_t         : __ChsLen,                             \
  Wcs_t               : __WcsLen,                             \
  const Wcs_t         : __WcsLen,                             \
  nString_ptr         : String_Length,                        \
  const nString_ptr   : String_Length,                        \
  nStringAry_t        : StringAry_Length,                     \
  const nStringAry_t  : StringAry_Length                      \
  ) (Instance)

// nString_ptr        String_toString_Bool              (const bool pValue);
// nString_ptr        String_toString_Decimal           (const int64_t pValue);
// nString_ptr        String_toString_Decimal_Unsigned  (const uint64_t pValue);
// nString_ptr        String_toString_Digit             (const double pValue, const int numDigit);
// nString_ptr        String_toString_StringAry         (const nStringAry_ptr pValue, const nString_ptr pReplaceWord);

// bool               String_valueOf_Bool               (const nString_ptr pSelf);
// int64_t            String_valueOf_Decimal            (const nString_ptr pSelf);
// uint64_t           String_valueOf_Decimal_Unsigned   (const nString_ptr pSelf);
// double             String_valueOf_Digit              (const nString_ptr pSelf);

Wcs_t              String_UTF8Decoder                 (Chs_t pValue, Length_t* out_pValueSize);
Chs_t              String_UTF8Encoder                 (Wcs_t pValue, Length_t* out_pValueSize);

#define nRegExp(RegExp, args...) _Generic((Instance),   \
  nString_t         : RegExp_Constructor_Str,           \
  nString_ptr       : RegExp_Constructor_Str,           \
  const nString_ptr : RegExp_Constructor_Str,           \
  nRegExp_t         : RegExp_Constructor_Clone,         \
  nRegExp_ptr       : RegExp_Constructor_Clone,         \
  const nRegExp_ptr : RegExp_Constructor_Clone)         \
  (RegExp, ##args)

nRegExp_ptr        RegExp_Constructor_Str             (nString_ptr pRegExp, nString_ptr pFlag);
nRegExp_ptr        RegExp_Constructor_Clone           (const nRegExp_ptr  pSelf);
bool               RegExp_Destructor                  (      nRegExp_ptr* pSelf);
nRegExpResult_ptr  RegExp_Analysis                    (const nString_ptr  pSelf, const nRegExp_ptr pRegExp);
Length_t           RegExp_Count                       (const nString_ptr  pSelf, const nRegExp_ptr pRegExp);
Index_t            RegExp_IndexOf                     (const nString_ptr  pSelf, const nRegExp_ptr pRegExp);
Index_t            RegExp_IndexAt                     (const nString_ptr  pSelf, const nRegExp_ptr pRegExp, const Index_t pIndex);
Index_t            RegExp_IndexFor                    (const nString_ptr  pSelf, const nRegExp_ptr pRegExp, const Index_t pStart);
Index_t*           RegExp_IndexAll                    (const nString_ptr  pSelf, const nRegExp_ptr pRegExp);
Index_t            RegExp_LastOfIndex                 (const nString_ptr  pSelf, const nRegExp_ptr pRegExp);
nString_ptr        RegExp_Replace                     (const nString_ptr  pSelf, const nRegExp_ptr pRegExp, const nString_ptr pValue);
nString_ptr        RegExp_ReplaceAt                   (const nString_ptr  pSelf, const nRegExp_ptr pRegExp, const nString_ptr pValue, const Index_t pIndex);
nString_ptr        RegExp_ReplaceAll                  (const nString_ptr  pSelf, const nRegExp_ptr pRegExp, const nString_ptr pValue);
nStringAry_ptr     RegExp_Split                       (const nString_ptr  pSelf, const nRegExp_ptr pRegExp);
nString_ptr        RegExp_SubString                   (const nString_ptr  pSelf, const nRegExp_ptr pRegExp);

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