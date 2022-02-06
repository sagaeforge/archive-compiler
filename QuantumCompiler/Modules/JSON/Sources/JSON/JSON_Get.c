
#include <Exception.h>
#include <Json.h>
#include <Object.h>
#include <Private_Json.h>
#include <String.h>
#include <StringLib.h>

Object
JSON_Get(const JSONObject pSelf, const String pFieldName)
{
  JSONNode node = pSelf->m_Nodes;

  Index_t i;
  for (i = 0; i < pSelf->m_FieldLength; i++) {
    if (StringMethod.Compare(node->m_Name, pFieldName)) {
      switch (node->m_DataType) {
        case JSONDataType_Decimal:
          return Object(ValueOf(long long)(node->m_Value.StringValue));
        case JSONDataType_Digit:
          return Object(ValueOf(double)(node->m_Value.StringValue));
        case JSONDataType_Boolean:
          return Object(ValueOf(bool)(node->m_Value.StringValue));
        case JSONDataType_String:
          return Object(node->m_Value.StringValue);
        case JSONDataType_JSONObject:
          return Object((JSONObject)node->m_Value.ReferenceValue);
        case JSONDataType_Ary:
          return Object((JSONAry)node->m_Value.ReferenceValue);
        case JSONDataType_NULL:
          return Object(NULL);
        default:
          return NULL;
      }
    }
    node = node->Next;
  }

  Exception(ERROR, "해당 필드가 없습니다. [field:%S]", pFieldName->m_Value);
  return NULL;
}
