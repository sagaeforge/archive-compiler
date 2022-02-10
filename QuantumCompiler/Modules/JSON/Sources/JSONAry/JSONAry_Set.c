
#include <Exception.h>
#include <Json.h>
#include <Object.h>
#include <Private_JsonAry.h>
#include <StringLib.h>

bool
JSONAry_Set(JSONAry pSelf, const Index_t pIndex, const Object pValue)
{
  JSONAryNode node = pSelf->m_Nodes;
  if (node == NULL) {
    Exception(
      ERROR, "잘못된 인덱스 지정 [size:%u, idx:%u]", pSelf->m_Length, pIndex);
    return false;
  }
  Index_t LastIndex = pIndex;
  if (pSelf->m_Length <= LastIndex)
    LastIndex = pSelf->m_Length - 1;

  Index_t i;
  for (i = 0; i < LastIndex; i++) {
    node = node->Next;
  }

  void* value_void;
  switch (pValue->m_Info->m_Code) {
    case DataType_Int:
      node->m_Value.m_DataType = JSONDataType_Decimal;
      node->m_Value.m_Value.StringValue = toString(UnBoxing(int)(pValue));
      break;
    case DataType_Long:
      node->m_Value.m_DataType = JSONDataType_Decimal;
      node->m_Value.m_Value.StringValue = toString(UnBoxing(long)(pValue));
      break;
    case DataType_Long_Long:
      node->m_Value.m_DataType = JSONDataType_Decimal;
      node->m_Value.m_Value.StringValue = toString(UnBoxing(long long)(pValue));
      break;
    case DataType_U_Int:
      node->m_Value.m_DataType = JSONDataType_Decimal;
      node->m_Value.m_Value.StringValue =
        toString(UnBoxing(unsigned int)(pValue));
      break;
    case DataType_U_Long:
      node->m_Value.m_DataType = JSONDataType_Decimal;
      node->m_Value.m_Value.StringValue =
        toString(UnBoxing(unsigned long)(pValue));
      break;
    case DataType_U_Long_Long:
      node->m_Value.m_DataType = JSONDataType_Decimal;
      node->m_Value.m_Value.StringValue =
        toString(UnBoxing(unsigned long long)(pValue));
      break;
    case DataType_Float:
      node->m_Value.m_DataType = JSONDataType_Digit;
      node->m_Value.m_Value.StringValue = toString(UnBoxing(float)(pValue));
      break;
    case DataType_Double:
      node->m_Value.m_DataType = JSONDataType_Digit;
      node->m_Value.m_Value.StringValue = toString(UnBoxing(double)(pValue));
      break;
    case DataType_Bool:
      node->m_Value.m_DataType = JSONDataType_Boolean;
      node->m_Value.m_Value.StringValue = toString(UnBoxing(bool)(pValue));
      break;
    case DataType_String:
      node->m_Value.m_DataType = JSONDataType_String;
      node->m_Value.m_Value.StringValue = UnBoxing(String_t*)(pValue);
      break;
    case DataType_JSONObject:
      node->m_Value.m_DataType = JSONDataType_JSONObject;
      node->m_Value.m_Value.ReferenceValue = UnBoxing(JSONObject)(pValue);
      break;
    case DataType_JSONAry:
      node->m_Value.m_DataType = JSONDataType_Ary;
      node->m_Value.m_Value.ReferenceValue = UnBoxing(JSONAry)(pValue);
      break;
    case DataType_Ptr_Void:
      value_void = UnBoxing(void*)(pValue);
      if (value_void != NULL) {
        Exception(ERROR,
                  "지원하지 않는 자료형입니다. [type:NULL이 아닌 void *형]");
        return false;
      }
      node->m_Value.m_DataType = JSONDataType_NULL;
      node->m_Value.m_Value.StringValue = String("NULL");
      break;
    default:
      Exception(
        ERROR, "지원하지 않는 자료형입니다. [type:%s]", pValue->m_Info->m_Name);
      return false;
  }
  return true;
}
