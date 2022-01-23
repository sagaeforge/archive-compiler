
#include "Object.h"
#include "Private_GarbageCollection.h"

__attribute__((warn_unused_result)) const Object
__Object_Boxing_Short(const short pValue)
{
  short* Value = Excute_MemoryCreate(sizeof(short));
  *Value = pValue;
  return GetObject(&g_DataTypeTable[DataType_Short], Value);
}
