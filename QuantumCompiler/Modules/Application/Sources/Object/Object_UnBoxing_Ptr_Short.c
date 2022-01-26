
#include "Object.h"
#include "Private_GarbageCollection.h"

short*
__Object_UnBoxing_Ptr_Short(const Object pSelf)
{
  short* value = (short*)pSelf->m_Value;
  Excute_MemoryRemove(pSelf->m_Value);
  FreeObject(pSelf);
  return value;
}
