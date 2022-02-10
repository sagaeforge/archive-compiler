
#include <Exception.h>
#include <Object.h>
#include <Private_JsonAry.h>
#include <StringLib.h>

bool
JSONAry_Insert(JSONAry pSelf, const Index_t pIndex, const Object pValue)
{
  JSONAryNode MakeNode = JSONAry_NodeCreate();
  void* value_void;
  switch (pValue->m_Info->m_Code) {
    case DataType_Int:
      MakeNode->m_Value.m_DataType = JSONDataType_Decimal;
      MakeNode->m_Value.m_Value.StringValue = toString(UnBoxing(int)(pValue));
      break;
    case DataType_Long:
      MakeNode->m_Value.m_DataType = JSONDataType_Decimal;
      MakeNode->m_Value.m_Value.StringValue = toString(UnBoxing(long)(pValue));
      break;
    case DataType_Long_Long:
      MakeNode->m_Value.m_DataType = JSONDataType_Decimal;
      MakeNode->m_Value.m_Value.StringValue =
        toString(UnBoxing(long long)(pValue));
      break;
    case DataType_U_Int:
      MakeNode->m_Value.m_DataType = JSONDataType_Decimal;
      MakeNode->m_Value.m_Value.StringValue =
        toString(UnBoxing(unsigned int)(pValue));
      break;
    case DataType_U_Long:
      MakeNode->m_Value.m_DataType = JSONDataType_Decimal;
      MakeNode->m_Value.m_Value.StringValue =
        toString(UnBoxing(unsigned long)(pValue));
      break;
    case DataType_U_Long_Long:
      MakeNode->m_Value.m_DataType = JSONDataType_Decimal;
      MakeNode->m_Value.m_Value.StringValue =
        toString(UnBoxing(unsigned long long)(pValue));
      break;
    case DataType_Float:
      MakeNode->m_Value.m_DataType = JSONDataType_Digit;
      MakeNode->m_Value.m_Value.StringValue = toString(UnBoxing(float)(pValue));
      break;
    case DataType_Double:
      MakeNode->m_Value.m_DataType = JSONDataType_Digit;
      MakeNode->m_Value.m_Value.StringValue =
        toString(UnBoxing(double)(pValue));
      break;
    case DataType_Bool:
      MakeNode->m_Value.m_DataType = JSONDataType_Boolean;
      MakeNode->m_Value.m_Value.StringValue = toString(UnBoxing(bool)(pValue));
      break;
    case DataType_String:
      MakeNode->m_Value.m_DataType = JSONDataType_String;
      MakeNode->m_Value.m_Value.StringValue = UnBoxing(String_t*)(pValue);
      break;
    case DataType_JSONObject:
      MakeNode->m_Value.m_DataType = JSONDataType_JSONObject;
      MakeNode->m_Value.m_Value.ReferenceValue = UnBoxing(JSONObject)(pValue);
      break;
    case DataType_JSONAry:
      MakeNode->m_Value.m_DataType = JSONDataType_Ary;
      MakeNode->m_Value.m_Value.ReferenceValue = UnBoxing(JSONAry)(pValue);
      break;
    case DataType_Ptr_Void:
      value_void = UnBoxing(void*)(pValue);
      if (value_void != NULL) {
        Exception(ERROR,
                  "지원하지 않는 자료형입니다. [type:NULL이 아닌 void *형]");
        return false;
      }
      MakeNode->m_Value.m_DataType = JSONDataType_NULL;
      MakeNode->m_Value.m_Value.StringValue = String("NULL");
      break;
    default:
      Exception(
        ERROR, "지원하지 않는 자료형입니다. [type:%s]", pValue->m_Info->m_Name);
      return false;
  }

  JSONAryNode node = pSelf->m_Nodes;
  if (node == NULL) {
    if (pIndex != 0) {
      Exception(ERROR, "잘못된 인덱스를 지정했습니다. [idx:%u]", pIndex);
      return false;
    }
    // 처음
    pSelf->m_Nodes = MakeNode;
    pSelf->m_Length++;
    return true;
  }
  Index_t LastIndex = pIndex;
  if (pSelf->m_Length <= LastIndex)
    LastIndex = pSelf->m_Length - 1;

  Index_t i;
  for (i = 0; i < LastIndex; i++)
    node = node->Next;

  JSONAryNode back = node->Next;
  node->Next = MakeNode;
  MakeNode->Next = back;
  pSelf->m_Length++;

  return true;
}
