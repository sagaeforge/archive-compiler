
#include "Object.h"
#include "Private_GarbageCollection.h"

int**
__Object_UnBoxing_Double_Ptr_Int(const Object pSelf)
{
  int** value = (int**)pSelf->m_Value;
  Excute_MemoryRemove(pSelf->m_Value);
  FreeObject(pSelf);
  return value;
}
