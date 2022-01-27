
#include <GarbageCollection.h>
#include <Private_GarbageCollection.h>

#include <stdlib.h>

void
Excute_MemoryRemove(void* pObj)
{
  GarbageCollection_Remove(pObj);
  free(pObj);
}

void
MemoryRemove(void* pObj)
{
  Excute_MemoryRemove(pObj);
}