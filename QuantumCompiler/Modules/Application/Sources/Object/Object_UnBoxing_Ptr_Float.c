
#include "Object.h"
#include "Private_GarbageCollection.h"

float*
__Object_UnBoxing_Ptr_Float(const Object pSelf)
{
  float* value = (float*)pSelf->m_Value;
  Excute_MemoryRemove(pSelf->m_Value);
  FreeObject(pSelf);
  return value;
}
