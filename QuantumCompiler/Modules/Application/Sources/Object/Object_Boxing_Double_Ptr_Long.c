
#include "Object.h"
#include "Private_GarbageCollection.h"

__attribute__((warn_unused_result)) const Object
__Object_Boxing_Double_Ptr_Long(const long** pValue)
{
  long** Value = Excute_MemoryCreate(sizeof(void*));
  Value = (long**)pValue;
  return GetObject(&g_DataTypeTable[DataType_Double_Ptr_Long], Value);
}
