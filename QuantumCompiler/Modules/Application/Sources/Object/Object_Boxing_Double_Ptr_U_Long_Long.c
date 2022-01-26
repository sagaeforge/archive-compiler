
#include "Object.h"
#include "Private_GarbageCollection.h"

__attribute__((warn_unused_result)) const Object
__Object_Boxing_Double_Ptr_U_Long_Long(const unsigned long long** pValue)
{
  unsigned long long** Value = Excute_MemoryCreate(sizeof(void*));
  Value = (unsigned long long**)pValue;
  return GetObject(&g_DataTypeTable[DataType_Double_Ptr_U_Long_Long], Value);
}
