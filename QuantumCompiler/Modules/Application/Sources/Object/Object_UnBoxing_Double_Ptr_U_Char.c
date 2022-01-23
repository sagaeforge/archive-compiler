
#include "Object.h"
#include "Private_GarbageCollection.h"

unsigned char**
__Object_UnBoxing_Double_Ptr_U_Char(const Object pSelf)
{
  unsigned char** value = (unsigned char**)pSelf->m_Value;
  Excute_MemoryRemove(pSelf->m_Value);
  FreeObject(pSelf);
  return value;
}
