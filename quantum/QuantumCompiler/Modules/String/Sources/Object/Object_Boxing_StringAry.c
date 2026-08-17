
#include <GarbageCollection.h>
#include <Object.h>
#include <String.h>

__attribute__((warn_unused_result)) const Object
__Object_Boxing_StringAry(const StringAry pValue)
{
  StringAry Value = MemoryCreate(sizeof(void*));
  Value = (StringAry)pValue;
  return GetObject(&g_DataTypeTable[DataType_StringAry], Value);
}
