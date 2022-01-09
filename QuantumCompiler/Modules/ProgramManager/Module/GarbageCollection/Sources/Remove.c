
#include "Exception.h"
#include "GarbageCollection.h"
#include "Private_GarbageCollection.h"
#include <stdlib.h>

void
MemoryRemove(void** ptr)
{
  GC_Remove((*ptr));
  free((*ptr));
  (*ptr) = NULL;
}
