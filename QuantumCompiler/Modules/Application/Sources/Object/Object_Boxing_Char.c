
#include <Object.h>
#include <Private_GarbageCollection.h>

__attribute__((warn_unused_result)) const Object
__Object_Boxing_Char(const char pValue)
{
  char* Value = Excute_MemoryCreate(sizeof(char));
  *Value = pValue;
  return GetObject(&g_DataTypeTable[DataType_Char], Value);
}
