
#include <Object.h>
#include <Private_GarbageCollection.h>

__attribute__((warn_unused_result)) const Object
__Object_Boxing_Double(const double pValue)
{
  double* Value = Excute_MemoryCreate(sizeof(double));
  *Value = pValue;
  return GetObject(&g_DataTypeTable[DataType_Double], Value);
}
