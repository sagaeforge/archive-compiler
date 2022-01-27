
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
  JSONDataType_Ary,
  JSONDataType_NULL,
  JSONDataType_None
} JSONDataType;

#pragma pack(push, 1)

// clang-format off

typedef struct
{
  String              m_Name;
  String              m_Content;
  void*               m_Value;
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