
#include <Exception.h>
#include <Private_JsonAry.h>
#include <StringLib.h>

bool
JSONAry_Push(JSONAry pSelf, const Object pValue)
{
  JSONAryNode MakeNode = JSONAry_NodeCreate();

  DataTypeCode_t DTC = DataType_None;
  int64_t value_int = 0;
  uint64_t value_uint = 0;
  double value_double = 0;
  String value_string = NULL;
  bool value_boolean = false;
  JSONObject value_jsonobject = NULL;

  switch (pValue->m_Info->m_Code) {
      // clang-format off
    case DataType_Int:          value_int = UnBoxing(int)(pValue);                  DTC = DataType_Int;        break;
    case DataType_U_Int:        value_uint = UnBoxing(unsigned int)(pValue);        DTC = DataType_Int;        break;
    case DataType_Long:         value_int = UnBoxing(long)(pValue);                 DTC = DataType_Int;        break;
    case DataType_U_Long:       value_uint = UnBoxing(unsigned long)(pValue);       DTC = DataType_Int;        break;
    case DataType_Long_Long:    value_int = UnBoxing(long long)(pValue);            DTC = DataType_Int;        break;
    case DataType_U_Long_Long:  value_uint = UnBoxing(unsigned long long)(pValue);  DTC = DataType_Int;        break;
    case DataType_Float:        value_double = UnBoxing(float)(pValue);             DTC = DataType_Float;      break;
    case DataType_Double:       value_double = UnBoxing(double)(pValue);            DTC = DataType_Float;      break;
    case DataType_Bool:         value_boolean = UnBoxing(bool)(pValue);             DTC = DataType_Bool;       break;
    case DataType_String:       value_string = UnBoxing(String_t *)(pValue);        DTC = DataType_String;     break;
    case DataType_JSONObject:   value_jsonobject = UnBoxing(JSONObject)(pValue);    DTC = DataType_JSONObject; break;
      // clang-format on
    default:
      Exception(
        ERROR, "지원하지 않는 자료형입니다. [type%s]", pValue->m_Info->m_Name);
      return false;
  }

  if (DTC == DataType_Int) {
    MakeNode->m_Value.m_DataType = JSONDataType_Decimal;
    if (value_int != 0) {
      MakeNode->m_Value.m_Value.StringValue = toString(value_int);
    } else {
      MakeNode->m_Value.m_Value.StringValue = toString(value_uint);
    }
  } else if (DTC == DataType_Float) {
    MakeNode->m_Value.m_DataType = JSONDataType_Digit;
    MakeNode->m_Value.m_Value.StringValue = toString(value_double);
  } else if (DTC == DataType_Bool) {
    MakeNode->m_Value.m_DataType = JSONDataType_Boolean;
    MakeNode->m_Value.m_Value.StringValue = toString(value_boolean);
  } else if (DTC == DataType_String) {
    MakeNode->m_Value.m_DataType = JSONDataType_String;
    MakeNode->m_Value.m_Value.StringValue = value_string;
  } else {
    MakeNode->m_Value.m_DataType = JSONDataType_JSONObject;
    MakeNode->m_Value.m_Value.ReferenceValue = value_jsonobject;
  }

  JSONAryNode Node = pSelf->m_Nodes;
  if (Node == NULL) {
    pSelf->m_Nodes = MakeNode;
  } else {
    while (Node->Next != NULL)
      Node = Node->Next;
    Node->Next = MakeNode;
  }
  pSelf->m_Length++;

  return true;
}
