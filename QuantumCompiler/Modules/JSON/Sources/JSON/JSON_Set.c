
#include <Exception.h>
#include <Json.h>
#include <Object.h>
#include <Private_Json.h>
#include <String.h>
#include <StringLib.h>

bool
JSON_Set(JSONObject pSelf, const String pFieldName, const Object pValue)
{
  JSONNode node = pSelf->m_Nodes;

  Index_t i;
  for (i = 0; i < pSelf->m_FieldLength; i++) {
    if (StringMethod.Compare(node->m_Name, pFieldName)) {
      void* value_void;
      switch (pValue->m_Info->m_Code) {
        case DataType_Int:
          node->m_DataType = JSONDataType_Decimal;
          node->m_Value.StringValue = toString(UnBoxing(int)(pValue));
          break;
        case DataType_Long:
          node->m_DataType = JSONDataType_Decimal;
          node->m_Value.StringValue = toString(UnBoxing(long)(pValue));
          break;
        case DataType_Long_Long:
          node->m_DataType = JSONDataType_Decimal;
          node->m_Value.StringValue = toString(UnBoxing(long long)(pValue));
          break;
        case DataType_U_Int:
          node->m_DataType = JSONDataType_Decimal;
          node->m_Value.StringValue = toString(UnBoxing(unsigned int)(pValue));
          break;
        case DataType_U_Long:
          node->m_DataType = JSONDataType_Decimal;
          node->m_Value.StringValue = toString(UnBoxing(unsigned long)(pValue));
          break;
        case DataType_U_Long_Long:
          node->m_DataType = JSONDataType_Decimal;
          node->m_Value.StringValue =
            toString(UnBoxing(unsigned long long)(pValue));
          break;
        case DataType_Float:
          node->m_DataType = JSONDataType_Digit;
          node->m_Value.StringValue = toString(UnBoxing(float)(pValue));
          break;
        case DataType_Double:
          node->m_DataType = JSONDataType_Digit;
          node->m_Value.StringValue = toString(UnBoxing(double)(pValue));
          break;
        case DataType_Bool:
          node->m_DataType = JSONDataType_Boolean;
          node->m_Value.StringValue = toString(UnBoxing(bool)(pValue));
          break;
        case DataType_String:
          node->m_DataType = JSONDataType_String;
          node->m_Value.StringValue = UnBoxing(String_t*)(pValue);
          break;
        case DataType_JSONObject:
          node->m_DataType = JSONDataType_JSONObject;
          node->m_Value.ReferenceValue = UnBoxing(JSONObject)(pValue);
          break;
        case DataType_JSONAry:
          node->m_DataType = JSONDataType_Ary;
          node->m_Value.ReferenceValue = UnBoxing(JSONAry)(pValue);
          break;
        case DataType_Ptr_Void:
          value_void = UnBoxing(void*)(pValue);
          if (value_void != NULL) {
            Exception(
              ERROR, "지원하지 않는 자료형입니다. [type:NULL이 아닌 void *형]");
            return false;
          }
          node->m_DataType = JSONDataType_NULL;
          node->m_Value.ReferenceValue = NULL;
          break;
        default:
          Exception(ERROR,
                    "지원하지 않는 자료형입니다. [type:%s]",
                    pValue->m_Info->m_Name);
          return false;
      }
    }
    node = node->Next;
  }

  Exception(ERROR, "해당 필드가 없습니다. [field:%S]", pFieldName->m_Value);
  return false;
}
