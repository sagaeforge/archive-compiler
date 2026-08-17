
#include <Exception.h>
#include <Json.h>
#include <Object.h>
#include <Private_Json.h>
#include <Private_JsonAry.h>
#include <StringLib.h>

bool
JSONAry_Contains(const JSONAry pSelf, Object pTarget)
{
  JSONDataType DTC = JSONDataType_None;
  int64_t value_int = 0;
  uint64_t value_uint = 0;
  double value_double = 0;
  String value_string = NULL;
  bool value_boolean = false;
  JSONObject value_jsonobject = NULL;
  JSONAry value_jsonary = NULL;
  void* value_void = NULL;

  switch (pTarget->m_Info->m_Code) {
      // clang-format off
    case DataType_Int:          value_int = UnBoxing(int)(pTarget);                  DTC = JSONDataType_Decimal;    break;
    case DataType_Long:         value_int = UnBoxing(long)(pTarget);                 DTC = JSONDataType_Decimal;    break;
    case DataType_Long_Long:    value_int = UnBoxing(long long)(pTarget);            DTC = JSONDataType_Decimal;    break;
    case DataType_U_Int:        value_uint = UnBoxing(unsigned int)(pTarget);        DTC = JSONDataType_Decimal;    break;
    case DataType_U_Long:       value_uint = UnBoxing(unsigned long)(pTarget);       DTC = JSONDataType_Decimal;    break;
    case DataType_U_Long_Long:  value_uint = UnBoxing(unsigned long long)(pTarget);  DTC = JSONDataType_Decimal;    break;
    case DataType_Float:        value_double = UnBoxing(float)(pTarget);             DTC = JSONDataType_Digit;      break;
    case DataType_Double:       value_double = UnBoxing(double)(pTarget);            DTC = JSONDataType_Digit;      break;
    case DataType_Bool:         value_boolean = UnBoxing(bool)(pTarget);             DTC = JSONDataType_Boolean;    break;
    case DataType_String:       value_string = UnBoxing(String_t*)(pTarget);         DTC = JSONDataType_String;     break;
    case DataType_JSONObject:   value_jsonobject = UnBoxing(JSONObject)(pTarget);    DTC = JSONDataType_JSONObject; break;
    case DataType_JSONAry:      value_jsonary = UnBoxing(JSONAry)(pTarget);          DTC = JSONDataType_Ary;        break;
    case DataType_Ptr_Void:     value_void = UnBoxing(void*)(pTarget);               DTC = JSONDataType_NULL;       break;
      // clang-format on
    default:
      Exception(ERROR,
                "지원하지 않는 자료형입니다. [type:%s]",
                pTarget->m_Info->m_Name);
      return false;
  }

  JSONAryNode node = pSelf->m_Nodes;
  String tempValue_Strng = NULL;
  while (node != NULL) {
    if (node->m_Value.m_DataType == DTC) {
      switch (DTC) {
        case JSONDataType_Decimal:
          if (value_int != 0)
            tempValue_Strng = toString(value_int);
          else
            tempValue_Strng = toString(value_uint);

          if (StringMethod.Compare(node->m_Value.m_Value.StringValue,
                                   tempValue_Strng))
            return true;
          break;
        case JSONDataType_Digit:
          tempValue_Strng = toString(value_double);
          if (StringMethod.Compare(node->m_Value.m_Value.StringValue,
                                   tempValue_Strng))
            return true;
          break;
        case JSONDataType_Boolean:
          tempValue_Strng = toString(value_boolean);
          if (StringMethod.Compare(node->m_Value.m_Value.StringValue,
                                   tempValue_Strng))
            return true;
          break;
        case JSONDataType_String:
          if (StringMethod.Compare(node->m_Value.m_Value.StringValue,
                                   value_string))
            return true;
          break;
        case JSONDataType_JSONObject:
          if (JSON_Compare((JSONObject)pSelf, value_jsonobject))
            return true;
          break;
        case JSONDataType_Ary:
          if (JSONAry_Compare((JSONAry)pSelf, value_jsonary))
            return true;
          break;
        case JSONDataType_NULL:
          if (StringMethod.Compare(node->m_Value.m_Value.StringValue,
                                   String("NULL")))
            return true;
          break;
        // clang-format off
        default: break;
          // clang-format on
      }
    }
    node = node->Next;
  }

  return false;
}
