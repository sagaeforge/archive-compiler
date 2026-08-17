
#include <Object.h>
#include <Private_GarbageCollection.h>

__attribute__((warn_unused_result)) const Object
__Object_Boxing_Double_Ptr_Long_Long(const long long** pValue)
{
  long long** Value = Excute_MemoryCreate(sizeof(void*));
  Value = (long long**)pValue;
  return GetObject(&g_DataTypeTable[DataType_Double_Ptr_Long_Long], Value);
}
