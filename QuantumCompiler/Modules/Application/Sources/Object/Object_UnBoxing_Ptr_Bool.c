
#include "Object.h"
#include "Private_GarbageCollection.h"

bool*
__Object_UnBoxing_Ptr_Bool(const Object pSelf)
{
  bool* value = (bool*)pSelf->m_Value;
  Excute_MemoryRemove(pSelf->m_Value);
  FreeObject(pSelf);
  return value;
}