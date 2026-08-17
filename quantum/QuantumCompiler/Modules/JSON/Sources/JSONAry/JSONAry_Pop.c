
#include <Json.h>
#include <Object.h>
#include <Private_JsonAry.h>
#include <StringLib.h>

Object
JSONAry_Pop(JSONAry pSelf)
{
  JSONAryNode node = pSelf->m_Nodes;
  if (node == NULL)
    return NULL;
  while (node->Next != NULL)
    node = node->Next;

  Object Obj = NULL;

  switch (node->m_Value.m_DataType) {
    case JSONDataType_Decimal:
      Obj = Object(ValueOf(long long)(node->m_Value.m_Value.StringValue));
    case JSONDataType_Digit:
      Obj = Object(ValueOf(double)(node->m_Value.m_Value.StringValue));
    case JSONDataType_Boolean:
      Obj = Object(ValueOf(bool)(node->m_Value.m_Value.StringValue));
    case JSONDataType_String:
      Obj = Object(node->m_Value.m_Value.StringValue);
    case JSONDataType_JSONObject:
      Obj = Object((JSONObject)node->m_Value.m_Value.ReferenceValue);
    case JSONDataType_Ary:
      Obj = Object((JSONAry)node->m_Value.m_Value.ReferenceValue);
    case JSONDataType_NULL:
      Obj = Object(NULL);
    default:
      break;
  }
  JSONAry_Remove(pSelf, pSelf->m_Length);
  return Obj;
}
