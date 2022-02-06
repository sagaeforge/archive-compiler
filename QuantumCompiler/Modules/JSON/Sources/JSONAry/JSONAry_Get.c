
#include <Exception.h>
#include <Json.h>
#include <Object.h>
#include <Private_JsonAry.h>
#include <StringLib.h>

Object
JSONAry_Get(const JSONAry pSelf, const Index_t pIndex)
{
  JSONAryNode node = pSelf->m_Nodes;
  if (node == NULL) {
    Exception(
      ERROR, "잘못된 인덱스 지정 [size:%u, idx:%u]", pSelf->m_Length, pIndex);
    return NULL;
  }
  Index_t LastIndex = pIndex;
  if (pSelf->m_Length <= LastIndex)
    LastIndex = pSelf->m_Length - 1;

  Index_t i;
  for (i = 0; i < LastIndex; i++) {
    node = node->Next;
  }

  switch (node->m_Value.m_DataType) {
    case JSONDataType_Decimal:
      return Object(ValueOf(long long)(node->m_Value.m_Value.StringValue));
    case JSONDataType_Digit:
      return Object(ValueOf(double)(node->m_Value.m_Value.StringValue));
    case JSONDataType_Boolean:
      return Object(ValueOf(bool)(node->m_Value.m_Value.StringValue));
    case JSONDataType_String:
      return Object(node->m_Value.m_Value.StringValue);
    case JSONDataType_JSONObject:
      return Object((JSONObject)node->m_Value.m_Value.ReferenceValue);
    case JSONDataType_Ary:
      return Object((JSONAry)node->m_Value.m_Value.ReferenceValue);
    case JSONDataType_NULL:
      return Object(NULL);
    default:
      break;
  }
  return NULL;
}
