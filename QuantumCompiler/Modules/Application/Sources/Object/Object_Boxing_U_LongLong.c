
#include "Object.h"
#include "Private_GarbageCollection.h"

__attribute__((warn_unused_result)) const Object
__Object_Boxing_U_LongLong(const unsigned long long pValue)
{
  unsigned long long* Value = Excute_MemoryCreate(sizeof(unsigned long long));
  *Value = pValue;
  return GetObject(&g_DataTypeTable[DataType_U_Long_Long], Value);
}
