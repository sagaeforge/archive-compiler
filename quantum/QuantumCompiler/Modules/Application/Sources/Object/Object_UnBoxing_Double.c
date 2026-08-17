
#include <Object.h>
#include <Private_GarbageCollection.h>

double
__Object_UnBoxing_Double(const Object pSelf)
{
  double value = *(double*)pSelf->m_Value;
  Excute_MemoryRemove(pSelf->m_Value);
  FreeObject(pSelf);
  return value;
}
