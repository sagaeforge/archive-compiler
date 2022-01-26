
#include "Object.h"
#include "Private_GarbageCollection.h"

__attribute__((warn_unused_result)) const Object
__Object_Boxing_Bool(const bool pValue)
{
  bool* Value = Excute_MemoryCreate(sizeof(bool));
  *Value = pValue;
  return GetObject(&g_DataTypeTable[DataType_Char], Value);
}
