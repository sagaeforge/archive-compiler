
#include <Object.h>
#include <Private_GarbageCollection.h>

__attribute__((warn_unused_result)) const Object
__Object_Boxing_LongLong(const long long pValue)
{
  long long* Value = Excute_MemoryCreate(sizeof(long long));
  *Value = pValue;
  return GetObject(&g_DataTypeTable[DataType_Long_Long], Value);
}
