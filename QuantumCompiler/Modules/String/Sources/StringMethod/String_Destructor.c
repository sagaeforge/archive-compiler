
#include <GarbageCollection.h>
#include <Private_String.h>

void
String_Destructor(String* pSelf)
{
  MemoryRemove((*pSelf)->m_Value);
  (*pSelf)->m_IsNone = false;
  (*pSelf)->m_Length = 0;
  MemoryRemove((*pSelf));
}
