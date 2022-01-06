
#ifndef __PUBLIC_DATATYPES_STRING__
#define __PUBLIC_DATATYPES_STRING__

#include "DataTypes.h"

#include <wchar.h>

typedef char *chs;
typedef const char *const_chs;
typedef wchar_t *wcs;
typedef const wchar_t *const_wcs;

typedef struct _String {
  bool IsNone;
  bool IsConst;
  wcs Value;
  Length Length;
} String;

typedef struct _StringAry {
  struct _StringAryNode {
    String *Value;
    struct _StringAryNode *Next;
  } Values;

  Length Length;
} StringAry;

struct StringMethod {
  // clang-format off
    String     *(*Join)        (String*, String*);
    void        (*Append)      (String*, String*);
    String     *(*SubString)   (String*, String*);
    String     *(*Loop)        (String*, Length);
    StringAry  *(*Split)       (String*, String*);
    bool        (*Compare)     (String*, String*);
    String     *(*Trim)        (String*);
    bool        (*Contains)    (String*, String*);
    Length      (*Count)       (String*, String*);
    wcs         (*Get)         (String*);
    void        (*Set)         (String*, String*);
    Length      (*Length)      (String*);
    String     *(*ToLower)     (String*);
    String     *(*ToUpper)     (String*);
    bool        (*IsNone)      (String*);
    int         (*IndexOf)     (String*, String*);
    int         (*LastOfIndex) (String*, String*);
    String     *(*Replace)     (String*, String*, String*);
    String     *(*ReplaceFor)  (String*, String*, String*, Length);
    String     *(*ReplaceAll)  (String*, String*, String*);
    String     *(*Left)        (String*, Length);
    String     *(*Right)       (String*, Length);
    String     *(*Middle)      (String*, Index, Index);
    void        (*Const)       (String*);
    void        (*UnConst)     (String*);
    void        (*Destructor)  (String**);
};

#endif