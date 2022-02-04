
#include <GarbageCollection.h>
#include <Object.h>
#include <String.h>

StringAry
__Object_UnBoxing_StringAry(const Object pSelf)
{
  StringAry value = (StringAry)pSelf->m_Value;
  MemoryRemove(pSelf->m_Value);
  FreeObject(pSelf);
  return value;
}