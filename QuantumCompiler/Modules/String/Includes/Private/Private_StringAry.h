#ifndef __PRIVATE__STRINGARY__
#define __PRIVATE__STRINGARY__

#include <StringAry.h>

// clang-format off

typedef struct _StringAryNode StringAryNode;

StringAryNode*  StringAry_NodeCreate  ();
void            StringAryDestructor   (StringAry* pSelf);
String          StringAry_Get         (StringAry pSelf, Index_t pIndex);
void            StringAry_Insert      (StringAry pSelf, String pValue, Index_t pIndex);
void            StringAry_Remove      (StringAry pSelf, Index_t pIndex);
void            StringAry_Push        (StringAry pSelf, String pValue);
String          StringAry_Pop         (StringAry pSelf);
Index_t         StringAry_Search      (StringAry pSelf, String pValue);
Length_t        StringAry_Contains    (StringAry pSelf, String pValue);

#endif