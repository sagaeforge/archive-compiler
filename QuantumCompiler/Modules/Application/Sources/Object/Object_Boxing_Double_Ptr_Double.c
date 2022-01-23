
#include "Object.h"
#include "Private_GarbageCollection.h"

__attribute__((warn_unused_result)) const Object
__Object_Boxing_Double_Ptr_Double(const double** pValue)
{
  double** Value = Excute_MemoryCreate(sizeof(void*));
  Value = (double**)pValue;
  return GetObject(&g_DataTypeTable[DataType_Double_Ptr_Double], Value);
}
