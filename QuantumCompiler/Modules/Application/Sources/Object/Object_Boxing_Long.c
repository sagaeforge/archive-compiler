
#include "Object.h"
#include "Private_GarbageCollection.h"

__attribute__((warn_unused_result)) const Object
__Object_Boxing_Long(const long pValue)
{
  long* Value = Excute_MemoryCreate(sizeof(long));
  *Value = pValue;
  return GetObject(&g_DataTypeTable[DataType_Long], Value);
}
