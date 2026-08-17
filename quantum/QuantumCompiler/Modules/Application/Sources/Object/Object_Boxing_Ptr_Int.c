
#include <Object.h>
#include <Private_GarbageCollection.h>

__attribute__((warn_unused_result)) const Object
__Object_Boxing_Ptr_Int(const int* pValue)
{
  int* Value = Excute_MemoryCreate(sizeof(void*));
  Value = (int*)pValue;
  return GetObject(&g_DataTypeTable[DataType_Ptr_Int], Value);
}
