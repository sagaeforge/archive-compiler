
#ifndef __STRINGARY__
#define __STRINGARY__

#include "Types/DataTypes_String.h"

// clang-format off
#define StringAry(...)

StringAry  *StringAryConstructor  ();
StringAry  *StringAryDestructor   ();
String     *StringAry_Get         (StringAry *Self, Index Index);
void        StringAry_Insert      (StringAry *Self, Index Index);
void        StringAry_Remove      (StringAry *Self, Index Index);
void        StringAry_Push        (StringAry *Self, String *Value);
String     *StringAry_Pop         (StringAry *Self);
// clang-format on

#endif