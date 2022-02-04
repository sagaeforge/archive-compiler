
#ifndef __STRINGARY__
#define __STRINGARY__

#include <Types/DataTypes_String.h>

#define StringAry(Cnt, args...) StringAry_Constructor(Cnt, ##args)

StringAry
StringAry_Constructor(int pCnt, ...);
void
StringAry_Destructor(StringAry* pSelf);

extern struct StringAryMethod StringAryMethod;

#endif