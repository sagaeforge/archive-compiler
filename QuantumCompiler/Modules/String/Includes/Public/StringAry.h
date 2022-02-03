
#ifndef __STRINGARY__
#define __STRINGARY__

#include <Types/DataTypes_String.h>

#define StringAry(Cnt, args...) StringAryConstructor(Cnt, ##args)

StringAry
StringAryConstructor(int pCnt, ...);

extern struct StringAryMethod StringAryMethod;

#endif