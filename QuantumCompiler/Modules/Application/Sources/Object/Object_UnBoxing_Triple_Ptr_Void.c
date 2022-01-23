
#include "Object.h"
#include "Private_GarbageCollection.h"

void***
__Object_UnBoxing_Triple_Ptr_Void(const Object pSelf)
{
  void*** value = (void***)pSelf->m_Value;
  Excute_MemoryRemove(pSelf->m_Value);
  FreeObject(pSelf);
  return value;
}
