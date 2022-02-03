
#include <GarbageCollection.h>
#include <Private_String.h>

void
String_Destructor(String* pSelf)
{
  (*pSelf)->IsNone = false;
  (*pSelf)->Length = 0;
  MemoryRemove((*pSelf)->Value);
  MemoryRemove(pSelf);
}
