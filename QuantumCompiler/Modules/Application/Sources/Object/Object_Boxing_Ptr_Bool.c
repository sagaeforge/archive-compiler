
#include "Object.h"
#include "Private_GarbageCollection.h"

__attribute__((warn_unused_result)) const Object
__Object_Boxing_Ptr_Bool(const bool* pValue)
{
  bool* Value = Excute_MemoryCreate(sizeof(void*));
  Value = (bool*)pValue;
  return GetObject(&g_DataTypeTable[DataType_Ptr_Bool], Value);
}