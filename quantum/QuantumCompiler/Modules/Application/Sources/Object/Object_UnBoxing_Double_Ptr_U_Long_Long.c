
#include <Object.h>
#include <Private_GarbageCollection.h>

unsigned long long**
__Object_UnBoxing_Double_Ptr_U_Long_Long(const Object pSelf)
{
  unsigned long long** value = (unsigned long long**)pSelf->m_Value;
  Excute_MemoryRemove(pSelf->m_Value);
  FreeObject(pSelf);
  return value;
}
