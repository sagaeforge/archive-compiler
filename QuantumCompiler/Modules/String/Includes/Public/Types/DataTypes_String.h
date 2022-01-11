
#ifndef __PUBLIC_DATATYPES_STRING__
#define __PUBLIC_DATATYPES_STRING__

#include "DataTypes.h"

#include <wchar.h>

typedef char* chs;
typedef const char* const_chs;
typedef wchar_t* wcs;
typedef const wchar_t* const_wcs;

enum StringPolicy
{
  // claog-format off
  StringPolicy_None = 0,
  StringPolicy_Null = 1,
  StringPolicy_Const = 2
  // claog-format on
};

typedef struct _String
{
  unsigned char Policy;
  wcs Value;
  Length Length;
} String;

typedef struct _StringAry
{
  struct _StringAryNode
  {
    String* Value;
    struct _StringAryNode* Next;
  } * Values;
  Length Length;
} StringAry;

struct StringMethod
{
  // clang-format off
    String     *(*Join)             (String*, String*);
    void        (*Append)           (String*, String*);
    String     *(*SubString)        (String*, String*);
    String     *(*Loop)             (String*, Length);
    StringAry  *(*Split)            (String*, String*);
    bool        (*Compare)          (String*, String*);
    String     *(*Trim)             (String*);
    bool        (*Contains)         (String*, String*);
    Length      (*Count)            (String*, String*);
    wcs         (*Get)              (String*);
    void        (*Set)              (String*, String*);
    Length      (*Length)           (String*);
    String     *(*ToLower)          (String*);
    String     *(*ToUpper)          (String*);
    bool        (*IsNone)           (String*);
    int         (*IndexOf)          (String*, String*);
    Index       (*IndexFor)         (String *, String *, Index);
    int         (*LastOfIndex)      (String*, String*);
    String     *(*Replace)          (String*, String*, String*);
    String     *(*ReplaceFor)       (String*, String*, String*, Length);
    String     *(*ReplaceAll)       (String*, String*, String*);
    String     *(*Left)             (String*, Length);
    String     *(*Right)            (String*, Length);
    String     *(*Middle)           (String*, Index, Index);
    void        (*Const)            (String*);
    void        (*UnConst)          (String*);
    void        (*Destructor)       (String**);
};

struct StringLibMethod {
  String *(*Format)           (String *, ...);
  bool    (*Pattern)          (String *, String *Format);
  String *(*Extract)          (String *, Index, Index);
  String *(*Notation)         (_int64, int);
  String *(*Reverse)          (String *);
  bool    (*IsAlpha)          (String *);
  bool    (*IsLower)          (String *);
  bool    (*IsUpper)          (String *);
  bool    (*IsDecimal)        (String *);
  bool    (*IsDigit)          (String *);
  bool    (*IsSpace)          (String *);
  bool    (*IsAlphaDigit)     (String *);
  bool    (*IsHex)            (String *);
  bool    (*IsControl)        (String *);
  bool    (*IsOctal)          (String *);
  bool    (*IsBinary)         (String *);
};

struct StringAryMethod
{
  void      (*Destructor)  (StringAry**);
  String*   (*Get)        (StringAry*, Index);
  void      (*Insert)     (StringAry*, String*, Index);
  void      (*Remove)     (StringAry*, Index);
  void      (*Push)       (StringAry*, String*);
  String*   (*Pop)        (StringAry*);
  Index     (*Search)     (StringAry*, String*);
  Length    (*Contains)   (StringAry*, String*);
};


#endif