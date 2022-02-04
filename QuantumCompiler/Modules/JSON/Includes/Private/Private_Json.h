
#ifndef __PRIVATE_JSON_JSON__
#define __PRIVATE_JSON_JSON__

#include <Types/DataType_Json.h>

// clang-format off
JSONNode JSON_NodeCreate();

Object        JSON_Get                (const JSONObject pSelf, const String pFieldName);
int64_t       JSON_GetDeciaml         (const JSONObject pSelf, const String pFieldName);
double        JSON_GetDigit           (const JSONObject pSelf, const String pFieldName);
bool          JSON_GetBool            (const JSONObject pSelf, const String pFieldName);
String        JSON_GetString          (const JSONObject pSelf, const String pFieldName);
JSONObject    JSON_GetObject          (const JSONObject pSelf, const String pFieldName);
void*         JSON_GetAry             (const JSONObject pSelf, const String pFieldName);
void*         JSON_GetNULL            (const JSONObject pSelf, const String pFieldName);

bool          JSON_Set                (JSONObject pSelf, const String pFieldName, const Object      pValue);
bool          JSON_SetDeciaml         (JSONObject pSelf, const String pFieldName, const int64_t     pValue);
bool          JSON_SetDigit           (JSONObject pSelf, const String pFieldName, const double      pValue);
bool          JSON_SetBool            (JSONObject pSelf, const String pFieldName, const bool        pValue);
bool          JSON_SetString          (JSONObject pSelf, const String pFieldName, const String      pValue);
bool          JSON_SetObject          (JSONObject pSelf, const String pFieldName, const JSONObject  pValue);
bool          JSON_SetAry             (JSONObject pSelf, const String pFieldName, const void*       pValue);
bool          JSON_SetNULL            (JSONObject pSelf, const String pFieldName);

bool          JSON_Append             (JSONObject pSelf, const String pFieldName, const Object      pValue);
bool          JSON_AppendDeciaml      (JSONObject pSelf, const String pFieldName, const int64_t     pValue);
bool          JSON_AppendDigit        (JSONObject pSelf, const String pFieldName, const double      pValue);
bool          JSON_AppendBool         (JSONObject pSelf, const String pFieldName, const bool        pValue);
bool          JSON_AppendString       (JSONObject pSelf, const String pFieldName, const String      pValue);
bool          JSON_AppendObject       (JSONObject pSelf, const String pFieldName, const JSONObject  pValue);
bool          JSON_AppendAry          (JSONObject pSelf, const String pFieldName, const void*       pValue);
bool          JSON_AppendNULL         (JSONObject pSelf, const String pFieldName);
bool          JSON_AppendDeciamlAry   (JSONObject pSelf, const String pFieldName, const int64_t*    pValue);
bool          JSON_AppendDigitAry     (JSONObject pSelf, const String pFieldName, const double*     pValue);
bool          JSON_AppendBoolAry      (JSONObject pSelf, const String pFieldName, const bool*       pValue);
bool          JSON_AppendStringAry    (JSONObject pSelf, const String pFieldName, const StringAry   pValue);
bool          JSON_AppendObjectAry    (JSONObject pSelf, const String pFieldName, const JSONObject  pValue);

bool          JSON_Remove             (JSONObject pSelf, const String pFieldName);
bool          JSON_RemoveAryAt        (JSONObject pSelf, const Index_t pIndex);

bool          JSON_IsTypeOf           (const JSONObject pSelf, const String     pFiledName, const JSONDataType pType);
bool          JSON_IsFieldOf          (const JSONObject pSelf, const String     pFiledName);
bool          JSON_Compare            (const JSONObject pSelf, const JSONObject pTarget);
JSONDataType  JSON_GetType            (const JSONObject pSelf, const String     pFiledName);
String        JSON_GetAryType         (const JSONObject pSelf, const String     pFiledName);
bool          JSON_Contains           (const JSONObject pSelf, const String     pFiledName);
JSONObject    JSON_Sum                (const JSONObject pSelf, const JSONObject pTarget, const int Policy);
StringAry     JSON_Export             (const JSONObject pSelf, const Length_t   TabSize);
JSONObject    JSON_GetParent          (const JSONObject pSelf);
Length_t      JSON_GetFiledLength     (const JSONObject pSelf);
JSONObject    JSON_Clone              (const JSONObject pSelf);
bool          JSON_Print              (const JSONObject pSelf);
bool          JSON_Clear              (const JSONObject pSelf);

#endif