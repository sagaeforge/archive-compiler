
#ifndef __PUBLIC_JSON_JSON__
#define __PUBLIC_JSON_JSON__

#include <Types/DataType_Json.h>
#include <Types/DataType_Object.h>

// clang-format off

JSONObject    JSON_Constructor        ();
JSONObject    JSON_Constructor_Parent (JSONObject pParent);
bool          JSON_Destructor         (JSONObject *pSelf);

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

__attribute__((warn_unused_result)) const Object          __Object_Boxing_JSONObject           (const JSONObject  pValue);
__attribute__((warn_unused_result)) const Object          __Object_Boxing_JSONAry              (const JSONAry     pValue);
                                    JSONObject            __Object_UnBoxing_JSONObject         (const Object      pSelf);
                                    JSONAry               __Object_UnBoxing_JSONAry            (const Object      pSelf);

extern struct _JSONMethod JSONMethod; 

#endif