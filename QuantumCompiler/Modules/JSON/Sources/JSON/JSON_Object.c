
#include <GarbageCollection.h>
#include <Json.h>
#include <Object.h>

__attribute__((warn_unused_result)) const Object
__Object_Boxing_JSONObject(const JSONObject pValue)
{
  JSONObject Value = MemoryCreate(sizeof(void*));
  Value = (JSONObject)pValue;
  return GetObject(&g_DataTypeTable[DataType_JSONObject], Value);
}

JSONObject
__Object_UnBoxing_JSONObject(const Object pSelf)
{
  JSONObject value = (JSONObject)pSelf->m_Value;
  MemoryRemove(pSelf->m_Value);
  FreeObject(pSelf);
  return value;
}

__attribute__((warn_unused_result)) const Object
__Object_Boxing_JSONAry(const JSONAry pValue)
{
  JSONAry Value = MemoryCreate(sizeof(void*));
  Value = (JSONAry)pValue;
  // TODO 처리
  return GetObject(&g_DataTypeTable[DataType_JSONObject], Value);
}

JSONAry
__Object_UnBoxing_JSONAry(const Object pSelf)
{
  JSONAry value = (JSONAry)pSelf->m_Value;
  MemoryRemove(pSelf->m_Value);
  FreeObject(pSelf);
  return value;
}