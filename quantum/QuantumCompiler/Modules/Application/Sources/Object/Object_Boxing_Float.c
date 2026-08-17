
#include <Object.h>
#include <Private_GarbageCollection.h>

__attribute__((warn_unused_result)) const Object
__Object_Boxing_Float(const float pValue)
{
  float* Value = Excute_MemoryCreate(sizeof(float));
  *Value = pValue;
  return GetObject(&g_DataTypeTable[DataType_Float], Value);
}
