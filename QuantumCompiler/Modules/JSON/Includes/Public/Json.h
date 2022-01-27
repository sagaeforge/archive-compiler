
#ifndef __PUBLIC_JSON_JSON__
#define __PUBLIC_JSON_JSON__

#include <Types/DataType_Json.h>

// clang-format off

JSONObject    JSON_Constructor        ();
bool          JSON_Destructor         (const JSONObject pSelf);


#define JSON_Read(Instance)                           \
  _Generic((Instance),                                \
  String    : JSON_Read_Str,                          \
  StringAry : JSON_Read_StrAry,                       \
  FILE *    : JSON_Read_File                          \
  ) (Instance)

JSONObject    JSON_Read_Str           (const String     pString);
JSONObject    JSON_Read_StrAry        (const StringAry  pStringAry);
JSONObject    JSON_Read_File          (const FILE *     pJsonFile);

Object        JSON_GetDeciaml         (const JSONObject pSelf, const String pFieldName);
Object        JSON_GetDigit           (const JSONObject pSelf, const String pFieldName);
Object        JSON_GetBool            (const JSONObject pSelf, const String pFieldName);
String        JSON_GetString          (const JSONObject pSelf, const String pFieldName);
JSONObject    JSON_GetObject          (const JSONObject pSelf, const String pFieldName);
Object        JSON_GetAry             (const JSONObject pSelf, const String pFieldName);
Object        JSON_GetNULL            (const JSONObject pSelf, const String pFieldName);

#define JSON_Write(Instance, Value)                   \
  _Generic((Instance),                                \
  String    : JSON_Write_Str,                         \
  StringAry : JSON_Write_StrAry,                      \
  FILE *    : JSON_Write_File                         \
  ) (Instance, Value)

bool          JSON_Write_Str          (String    *pString,    const JSONObject pSelf);
bool          JSON_Write_StrAry       (StringAry *pStringAry, const JSONObject pSelf);
bool          JSON_Write_File         (FILE *     pJsonFile,  const JSONObject pSelf);

bool          JSON_SetDeciaml         (JSONObject pSelf, const String pFieldName, Object        pValue);
bool          JSON_SetDigit           (JSONObject pSelf, const String pFieldName, Object        pValue);
bool          JSON_SetBool            (JSONObject pSelf, const String pFieldName, Object        pValue);
bool          JSON_SetString          (JSONObject pSelf, const String pFieldName, String        pValue);
bool          JSON_SetObject          (JSONObject pSelf, const String pFieldName, JSONObject    pValue);
bool          JSON_SetAry             (JSONObject pSelf, const String pFieldName, Object        pValue);
bool          JSON_SetNULL            (JSONObject pSelf, const String pFieldName, Object        pValue);

bool          JSON_AppendDeciaml      (JSONObject pSelf, const String pFieldName, Object        pValue);
bool          JSON_AppendDigit        (JSONObject pSelf, const String pFieldName, Object        pValue);
bool          JSON_AppendBool         (JSONObject pSelf, const String pFieldName, Object        pValue);
bool          JSON_AppendString       (JSONObject pSelf, const String pFieldName, String        pValue);
bool          JSON_AppendObject       (JSONObject pSelf, const String pFieldName, JSONObject    pValue);
bool          JSON_AppendAry          (JSONObject pSelf, const String pFieldName, Object        pValue);
bool          JSON_AppendNULL         (JSONObject pSelf, const String pFieldName, Object        pValue);

bool          JSON_Remove             (JSONObject pSelf, const String pFieldName);

bool          JSON_IsTypeOf           (const JSONObject pSelf, const String pFiledName, const JSONDataType pType);
bool          JSON_IsFieldOf          (const JSONObject pSelf, const String pFiledName);
bool          JSON_Compare            (const JSONObject pSelf, const JSONObject pSrc);
JSONDataType  JSON_GetType            (const JSONObject pSelf, const String pFiledName);
Length_t      JSON_GetFiledLength     (const JSONObject pSelf);
JSONObject    JSON_Find               (const JSONObject pSelf, const String pFiledName); // 자기 자신만
JSONObject    JSON_FindMap            (const JSONObject pSelf, const String pFiledName); // 자기 자신을 포함한 모든 곳을 재귀해서 검색
JSONObject    JSON_Clone              (const JSONObject pSelf);
JSONObject    JSON_Sum                (const JSONObject pSelf, const JSONObject pSrc, int Policy);
bool          JSON_Print              (const JSONObject pSelf);
StringAry     JSON_Export             (const JSONObject pSelf, Length_t TabSize);


#endif