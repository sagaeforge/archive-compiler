
#include "Object.h"
#include "Private_GarbageCollection.h"

unsigned int*
__Object_UnBoxing_Ptr_U_Int(const Object pSelf)
{
  unsigned int* value = (unsigned int*)pSelf->m_Value;
  Excute_MemoryRemove(pSelf->m_Value);
  FreeObject(pSelf);
  return value;
}
