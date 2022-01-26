
#include "Object.h"
#include "Private_GarbageCollection.h"

__attribute__((warn_unused_result)) const Object
__Object_Boxing_Double_Ptr_U_Short(const unsigned short** pValue)
{
  unsigned short** Value = Excute_MemoryCreate(sizeof(void*));
  Value = (unsigned short**)pValue;
  return GetObject(&g_DataTypeTable[DataType_Double_Ptr_U_Short], Value);
}
