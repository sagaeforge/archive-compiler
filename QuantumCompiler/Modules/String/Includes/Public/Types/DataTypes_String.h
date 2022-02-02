
#ifndef __PUBLIC_DATATYPES_STRING__
#define __PUBLIC_DATATYPES_STRING__

#include <Types/DataType.h>

#include <wchar.h>

typedef char* chs;
typedef const char* const_chs;
typedef wchar_t* wcs;
typedef const wchar_t* const_wcs;

#pragma pack(push, 1)
typedef struct _String
{
  bool IsNone;
  wcs Value;
  Length_t Length;
} String_t, *String;

typedef struct _StringAry
{
  struct _StringAryNode
  {
    String Value;
    struct _StringAryNode* Next;
  } * Values;
  Length_t Length;
} StringAry_t, *StringAry;

struct StringMethod
{
  // clang-format off
    String       (*Join)             (String, String);
    void         (*Append)           (String, String);
    String       (*SubString)        (String, String);
    String       (*Loop)             (String, Length_t);
    StringAry    (*Split)            (String, String);
    bool         (*Compare)          (String, String);
    String       (*Trim)             (String);
    bool         (*Contains)         (String, String);
    Length_t     (*Count)            (String, String);
    wcs          (*Get)              (String);
    void         (*Set)              (String, String);
    Length_t     (*Length)           (String);
    String       (*ToLower)          (String);
    String       (*ToUpper)          (String);
    bool         (*IsNone)           (String);
    Index_t      (*IndexOf)          (String, String);
    Index_t      (*IndexAt)          (String, String, Index_t);
    Index_t      (*IndexFor)         (String, String, Index_t);
    Index_t      (*LastOfIndex)      (String, String);
    String       (*Replace)          (String, String, String);
    String       (*ReplaceFor)       (String, String, String, Length_t);
    String       (*ReplaceAll)       (String, String, String);
    String       (*Left)             (String, Length_t);
    String       (*Right)            (String, Length_t);
    String       (*Middle)           (String, Index_t, Index_t);
    void         (*Destructor)       (String*);
};

struct StringLibMethod {
  String    (*Format)               (String, ...);
  bool      (*Pattern)              (String, String);
  String    (*Extract)              (String, Index_t, Index_t);
  String    (*Notation)             (int64_t, int);
  String    (*Reverse)              (String);
  StringAry (*FileAllRead)          (FILE*);
  bool      (*FileAllWrite)         (StringAry, FILE*);
  bool      (*IsAlpha)              (String);
  bool      (*IsLower)              (String);
  bool      (*IsUpper)              (String);
  bool      (*IsDecimal)            (String);
  bool      (*IsDigit)              (String);
  bool      (*IsSpace)              (String);
  bool      (*IsAlphaDigit)         (String);
  bool      (*IsHex)                (String);
  bool      (*IsControl)            (String);
  bool      (*IsOctal)              (String);
  bool      (*IsBinary)             (String);
};

struct StringAryMethod
{
  void      (*Destructor) (StringAry*);
  String    (*Get)        (StringAry, Index_t);
  void      (*Insert)     (StringAry, String, Index_t);
  void      (*Remove)     (StringAry, Index_t);
  void      (*Push)       (StringAry, String);
  String    (*Pop)        (StringAry);
  Index_t   (*Search)     (StringAry, String);
  Length_t  (*Contains)   (StringAry, String);
};

#pragma pack(pop)
#endif