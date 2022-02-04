
#ifndef __PUBLIC_JSON_JSON__
#define __PUBLIC_JSON_JSON__

#include <Types/DataType_Json.h>
#include <Types/DataType_Object.h>

// clang-format off

JSONObject    JSON_Constructor        ();
bool          JSON_Destructor         (const JSONObject *pSelf);

#define JSON_Read(Self, Instance)                     \
  _Generic((Instance),                                \
  String    : JSON_Read_Str,                          \
  StringAry : JSON_Read_StrAry,                       \
  FILE *    : JSON_Read_File                          \
  ) (Self, Instance)

#define JSON_Write(Instance, Value)                   \
  _Generic((Instance),                                \
  String    : JSON_Write_Str,                         \
  StringAry : JSON_Write_StrAry,                      \
  FILE *    : JSON_Write_File                         \
  ) (Instance, Value)

bool          JSON_Read_Str           (JSONObject pSelf,      const String     pString);
bool          JSON_Read_StrAry        (JSONObject pSelf,      const StringAry  pStringAry);
bool          JSON_Read_File          (JSONObject pSelf,      const FILE *     pJsonFile);

bool          JSON_Write_Str          (String    *pString,    const JSONObject pSelf);
bool          JSON_Write_StrAry       (StringAry *pStringAry, const JSONObject pSelf);
bool          JSON_Write_File         (FILE *     pJsonFile,  const JSONObject pSelf);

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
bool          JSON_Contains           (const JSONObject pSelf, const String     pFiledName);
Object        JSON_Find               (const JSONObject pSelf, const String     pFiledName); // 자기 자신만
Object        JSON_Search             (const JSONObject pSelf, const String     pFiledName); // 자기 자신을 포함한 모든 곳을 재귀해서 검색
JSONObject    JSON_Sum                (const JSONObject pSelf, const JSONObject pTarget, const int Policy);
StringAry     JSON_Export             (const JSONObject pSelf, const Length_t   TabSize);
Length_t      JSON_GetFiledLength     (const JSONObject pSelf);
JSONObject    JSON_Clone              (const JSONObject pSelf);
bool          JSON_Print              (const JSONObject pSelf);
bool          JSON_Clear              (const JSONObject pSelf);


__attribute__((warn_unused_result)) const Object          __Object_Boxing_JSONObject           (const JSONObject  pValue);
                                    JSONObject            __Object_UnBoxing_JSONObject         (const Object      pSelf);

#endif