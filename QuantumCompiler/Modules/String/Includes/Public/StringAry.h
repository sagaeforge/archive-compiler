
#ifndef __STRINGARY__
#define __STRINGARY__

#include "Types/DataTypes_String.h"

// clang-format off
#define StringAry(...)

StringAry  *StringAryConstructor      (int Cnt, ...);
StringAry  *StringAryDestructor       ();
String     *StringAry_Get             (StringAry *Self, Index Index);
void        StringAry_Insert          (StringAry *Self, Index Index);
void        StringAry_Remove          (StringAry *Self, Index Index);
void        StringAry_Push            (StringAry *Self, String *Value);
String     *StringAry_Pop             (StringAry *Self);
Index       StringAry_Search          (StringAry *Self, String *Value);
Length      StringAry_Contain         (StringAry *Self, String *Value);
// clang-format on

#endif