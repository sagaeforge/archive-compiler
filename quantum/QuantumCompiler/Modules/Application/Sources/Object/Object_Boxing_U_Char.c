
#include <Object.h>
#include <Private_GarbageCollection.h>

__attribute__((warn_unused_result)) const Object
__Object_Boxing_U_Char(const unsigned char pValue)
{
  unsigned char* Value = Excute_MemoryCreate(sizeof(unsigned char));
  *Value = pValue;
  return GetObject(&g_DataTypeTable[DataType_U_Char], Value);
}
