
#include "Object.h"
#include "Private_GarbageCollection.h"

char
__Object_UnBoxing_Char(const Object pSelf)
{
  char value = *(char*)pSelf->m_Value;
  Excute_MemoryRemove(pSelf->m_Value);
  FreeObject(pSelf);
  return value;
}
