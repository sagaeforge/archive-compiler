
#include "GarbageCollection.h"
#include "Private_String.h"

void
String_Destructor(String** Self)
{
  (*Self)->Policy = StringPolicy_None;
  (*Self)->Length = 0;
  MemoryRemove((void**)&(*Self)->Value);
  MemoryRemove((void**)Self);
}
