#ifndef __NSTRINGARY_H__
#define __NSTRINGARY_H__

#include <Module/Types/nString.h>

// clang-format off

#define StringAry(count, args...) StringAry_Constructor(Cnt, ##args)

nStringAry_t* StringAry_Constructor        (const int pCount, ...);
bool          StringAry_Destructor         (      nStringAry_t** pSelf);
bool          StringAry_Clear              (      nStringAry_t*  pSelf);
nString_t*    StringAry_Get                (const nStringAry_t*  pSelf, const Index_t pIndex);
bool          StringAry_Insert             (      nStringAry_t*  pSelf, const nString_t* pValue, Index_t pIndex);
bool          StringAry_Remove             (      nStringAry_t*  pSelf, const Index_t pIndex);
bool          StringAry_Push               (      nStringAry_t*  pSelf, const nString_t* pValue);
nString_t*    StringAry_Pop                (      nStringAry_t*  pSelf);
Index_t       StringAry_Search             (const nStringAry_t*  pSelf, const nString_t* pValue);
bool          StringAry_Contains           (const nStringAry_t*  pSelf, const nString_t* pValue);
nStringAry_t* StringAry_toAry              (const nStringAry_t*  pSelf);
nStringAry_t* StringAry_toList             (const nStringAry_t*  pSelf);
bool          StringAry_isAry              (const nStringAry_t*  pSelf);
bool          StringAry_isList             (const nStringAry_t*  pSelf);
bool          StringAry_Foreach            (bool (*CallBack)(nString_t*), void (*Last)(void));
Length_t      StringAry_CountIf            (bool (*Expression)(nString_t*));

#endif // __NSTRINGARY_H__