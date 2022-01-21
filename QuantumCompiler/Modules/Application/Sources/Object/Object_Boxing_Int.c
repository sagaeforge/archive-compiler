
#include "Object.h"
#include "Private_GarbageCollection.h"

// __attribute__((warn_unused_result)) const Object*
// __Object_Boxing_Int(const int pValue)
// {
//   ObjectValue Value;
//   Value.m_Value1d = (int*)&pValue;
//   Object* temp = GetObject(&g_SystemDataTypeTable[DataType_Int], Value);
//   return NULL;
// }
