
#include <GarbageCollection.h>
#include <Private_String.h>

void
String_Destructor(String* pSelf)
{
  MemoryRemove((*pSelf)->Value);
  (*pSelf)->IsNone = false;
  (*pSelf)->Length = 0;
  MemoryRemove((*pSelf));
}
