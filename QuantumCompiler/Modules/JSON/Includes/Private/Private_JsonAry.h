
#ifndef __PRIVATE_JSON_JSONARY__
#define __PRIVATE_JSON_JSONARY__

#include <Types/DataType_Json.h>

// clang-format off

JSONAryNode JSONAry_NodeCreate();

Object        JSONAry_Get                (const JSONAry  pSelf, const Index_t pIndex);
bool          JSONAry_Set                (      JSONAry  pSelf, const Index_t pIndex, const Object pValue);
bool          JSONAry_Insert             (      JSONAry  pSelf, const Index_t pIndex, const Object pValue);
bool          JSONAry_Remove             (      JSONAry  pSelf, const Index_t pIndex);
bool          JSONAry_Push               (      JSONAry  pSelf, const Object  pValue);
Object        JSONAry_Pop                (      JSONAry  pSelf);
bool          JSONAry_Compare            (const JSONAry  pSelf, const JSONAry pTarget);
bool          JSONAry_Contains           (const JSONAry  pSelf, const JSONAry pTarget);
bool          JSONAry_Clear              (      JSONAry  pSelf);
bool          JSONAry_Object             (const JSONAry  pSelf);
bool          JSONAry_SetObject          (const JSONAry  pSelf, const JSONObject pObject);
Length_t      JSONAry_Length             (const JSONAry  pSelf);

#endif