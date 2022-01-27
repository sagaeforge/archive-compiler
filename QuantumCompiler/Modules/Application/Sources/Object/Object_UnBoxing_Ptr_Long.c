
#include <Object.h>
#include <Private_GarbageCollection.h>

long*
__Object_UnBoxing_Ptr_Long(const Object pSelf)
{
  long* value = (long*)pSelf->m_Value;
  Excute_MemoryRemove(pSelf->m_Value);
  FreeObject(pSelf);
  return value;
}
