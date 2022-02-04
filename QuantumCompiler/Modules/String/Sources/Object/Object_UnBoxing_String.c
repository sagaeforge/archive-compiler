
#include <GarbageCollection.h>
#include <Object.h>
#include <String.h>

String
__Object_UnBoxing_String(const Object pSelf)
{
  String value = (String)pSelf->m_Value;
  MemoryRemove(pSelf->m_Value);
  FreeObject(pSelf);
  return value;
}