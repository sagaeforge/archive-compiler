
#ifndef __PUBLIC_JSON_JSONARY__
#define __PUBLIC_JSON_JSONARY__

#include <Types/DataType_Json.h>
#include <Types/DataType_Object.h>

// clang-format off

JSONAry       JSONAry_Constructor        ();
bool          JSONAry_Destructor         (JSONAry *pSelf);

extern struct _JSONAryMethod JSONAryMethod;

#endif