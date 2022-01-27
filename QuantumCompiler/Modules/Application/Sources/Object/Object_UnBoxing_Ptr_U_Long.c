
#include <Object.h>
#include <Private_GarbageCollection.h>

unsigned long*
__Object_UnBoxing_Ptr_U_Long(const Object pSelf)
{
  unsigned long* value = (unsigned long*)pSelf->m_Value;
  Excute_MemoryRemove(pSelf->m_Value);
  FreeObject(pSelf);
  return value;
}
