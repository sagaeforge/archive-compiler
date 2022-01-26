
#include "Object.h"
#include "Private_GarbageCollection.h"

__attribute__((warn_unused_result)) const Object
__Object_Boxing_U_Int(const unsigned int pValue)
{
  unsigned int* Value = Excute_MemoryCreate(sizeof(unsigned int));
  *Value = pValue;
  return GetObject(&g_DataTypeTable[DataType_U_Int], Value);
}
