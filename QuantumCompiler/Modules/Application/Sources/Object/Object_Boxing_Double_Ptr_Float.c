
#include "Object.h"
#include "Private_GarbageCollection.h"

__attribute__((warn_unused_result)) const Object
__Object_Boxing_Double_Ptr_Float(const float** pValue)
{
  float** Value = Excute_MemoryCreate(sizeof(void*));
  Value = (float**)pValue;
  return GetObject(&g_DataTypeTable[DataType_Double_Ptr_Float], Value);
}
