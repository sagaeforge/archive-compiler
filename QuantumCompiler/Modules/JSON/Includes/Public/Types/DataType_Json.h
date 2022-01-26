
#ifndef __PUBLIC_JSON_DATATYPE_JSON__
#define __PUBLIC_JSON_DATATYPE_JSON__

#include "String.h"

enum JSONDataType
{
  JSONDataType_Digit,
  JSONDataType_Boolean,
  JSONDataType_String,
  JSONDataType_JSONObject,
  JSONDataType_Ary,
  JSONDataType_NULL,
  JSONDataType_None
};

#pragma pack(push, 1)
typedef struct
{
  enum JSONDataType m_DataType;
  String m_Name;
  String m_Content;
  void* m_Value;
} JSONObject_t, *JSONObject;

typedef struct
{
  enum JSONDataType m_DataType;
  String m_Name;
  String m_Content;
  Index_t m_Length;
  JSONObject* m_Value;
} JSONObjectAry_t, *JSONObjectAry;

#pragma pack(pop)

#endif