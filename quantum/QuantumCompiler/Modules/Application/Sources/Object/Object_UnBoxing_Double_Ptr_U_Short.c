
#include <Object.h>
#include <Private_GarbageCollection.h>

unsigned short**
__Object_UnBoxing_Double_Ptr_U_Short(const Object pSelf)
{
  unsigned short** value = (unsigned short**)pSelf->m_Value;
  Excute_MemoryRemove(pSelf->m_Value);
  FreeObject(pSelf);
  return value;
}
