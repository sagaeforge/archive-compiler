
#include "Object.h"
#include "Private_GarbageCollection.h"

__attribute__((warn_unused_result)) const Object
__Object_Boxing_Ptr_Void(const void* pValue)
{
  void* Value = Excute_MemoryCreate(sizeof(void*));
  Value = (void*)pValue;
  return GetObject(&g_DataTypeTable[DataType_Ptr_Void], Value);
}
