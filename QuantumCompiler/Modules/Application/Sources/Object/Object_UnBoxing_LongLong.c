
#include "Object.h"
#include "Private_GarbageCollection.h"

long long
__Object_UnBoxing_LongLong(const Object pSelf)
{
  long long value = *(long long*)pSelf->m_Value;
  Excute_MemoryRemove(pSelf->m_Value);
  FreeObject(pSelf);
  return value;
}
