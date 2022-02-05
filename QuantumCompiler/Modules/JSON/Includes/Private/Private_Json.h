
#ifndef __PRIVATE_JSON_JSON__
#define __PRIVATE_JSON_JSON__

#include <Types/DataType_Json.h>

// clang-format off
JSONNode JSON_NodeCreate();

Object        JSON_Get                (const JSONObject pSelf, const String     pFieldName);
bool          JSON_Set                (      JSONObject pSelf, const String     pFieldName, const Object      pValue);
bool          JSON_Append             (      JSONObject pSelf, const String     pFieldName, const Object      pValue);
bool          JSON_Remove             (      JSONObject pSelf, const String     pFieldName);
bool          JSON_TypeOf             (const JSONObject pSelf, const String     pFiledName, const JSONDataType pType);
bool          JSON_FieldOf            (const JSONObject pSelf, const String     pFiledName);
bool          JSON_Compare            (const JSONObject pSelf, const JSONObject pTarget);
JSONDataType  JSON_Type               (const JSONObject pSelf, const String     pFiledName);
StringAry     JSON_Export             (const JSONObject pSelf, const Length_t   TabSize);
JSONObject    JSON_Parent             (const JSONObject pSelf);
bool          JSON_SetParent          (      JSONObject pSelf, const JSONObject pParent);
Length_t      JSON_FiledLength        (const JSONObject pSelf);
JSONObject    JSON_Clone              (const JSONObject pSelf);
bool          JSON_Print              (const JSONObject pSelf);
bool          JSON_Clear              (      JSONObject pSelf);

#endif