
#include <Object.h>
#include <Private_GarbageCollection.h>

__attribute__((warn_unused_result)) const Object
__Object_Boxing_U_Short(const unsigned short pValue)
{
  unsigned short* Value = Excute_MemoryCreate(sizeof(unsigned short));
  *Value = pValue;
  return GetObject(&g_DataTypeTable[DataType_U_Short], Value);
}
