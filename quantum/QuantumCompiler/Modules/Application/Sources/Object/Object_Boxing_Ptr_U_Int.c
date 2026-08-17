
#include <Object.h>
#include <Private_GarbageCollection.h>

__attribute__((warn_unused_result)) const Object
__Object_Boxing_Ptr_U_Int(const unsigned int* pValue)
{
  unsigned int* Value = Excute_MemoryCreate(sizeof(void*));
  Value = (unsigned int*)pValue;
  return GetObject(&g_DataTypeTable[DataType_Ptr_U_Int], Value);
}
