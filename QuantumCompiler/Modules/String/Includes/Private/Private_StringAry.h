#ifndef __PRIVATE__STRINGARY__
#define __PRIVATE__STRINGARY__

#include "StringAry.h"

typedef struct _StringAryNode StringAryNode;

StringAryNode*
StringAry_NodeCreate();

void
StringAryDestructor(StringAry** Self);
String*
StringAry_Get(StringAry* Self, Index Index);
void
StringAry_Insert(StringAry* Self, String* Value, Index Index);
void
StringAry_Remove(StringAry* Self, Index Index);
void
StringAry_Push(StringAry* Self, String* Value);
String*
StringAry_Pop(StringAry* Self);
Index
StringAry_Search(StringAry* Self, String* Value);
Length
StringAry_Contains(StringAry* Self, String* Value);

#endif