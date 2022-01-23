
#include "Object.h"
#include "Private_GarbageCollection.h"

__attribute__((warn_unused_result)) const Object
__Object_Boxing_Int(const int pValue)
{
  int* Value = Excute_MemoryCreate(sizeof(int));
  *Value = pValue;
  return GetObject(&g_DataTypeTable[DataType_Int], Value);
}
