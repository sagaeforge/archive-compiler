
#ifndef __PUBLIC_JSON_DATATYPE_JSON__
#define __PUBLIC_JSON_DATATYPE_JSON__

#include <Object.h>
#include <String.h>

typedef enum
{
  JSONDataType_Digit,
  JSONDataType_Decimal,
  JSONDataType_Boolean,
  JSONDataType_String,
  JSONDataType_JSONObject,
  JSONDataType_Digit_Ary,
  JSONDataType_Decimal_Ary,
  JSONDataType_Boolean_Ary,
  JSONDataType_String_Ary,
  JSONDataType_JSONObject_Ary,
  JSONDataType_NULL,
  JSONDataType_None
} JSONDataType;

// clang-format off
#pragma pack(push, 1)

typedef struct
{
  String              m_Name;
  String              m_Value;
  Length_t            m_Length;
  JSONDataType        m_DataType;
} JSONNode_t, *JSONNode;

typedef struct _JSONObject
{
  StringAry           m_FieldNames;
  Length_t            m_FieldLength;
  struct _JSONObject* m_Parent;
  JSONNode            m_Nodes;
} JSONObject_t, *JSONObject;

#pragma pack(pop)

#endif