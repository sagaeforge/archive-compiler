
#include <Object.h>
#include <Private_GarbageCollection.h>

__attribute__((warn_unused_result)) const Object
__Object_Boxing_Double_Ptr_U_Long(const unsigned long** pValue)
{
  unsigned long** Value = Excute_MemoryCreate(sizeof(void*));
  Value = (unsigned long**)pValue;
  return GetObject(&g_DataTypeTable[DataType_Double_Ptr_U_Long], Value);
}
