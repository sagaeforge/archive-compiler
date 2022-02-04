
#include <GarbageCollection.h>
#include <Object.h>
#include <String.h>

__attribute__((warn_unused_result)) const Object
__Object_Boxing_String(const String pValue)
{
  String Value = MemoryCreate(sizeof(void*));
  Value = (String)pValue;
  return GetObject(&g_DataTypeTable[DataType_String], Value);
}