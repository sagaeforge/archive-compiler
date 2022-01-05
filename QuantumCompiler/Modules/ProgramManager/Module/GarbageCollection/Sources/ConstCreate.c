

#include "Exception.h"
#include "GarbageCollection.h"
#include "Private_GarbageCollection.h"
#include <stdlib.h>

void *MemoryConstCreate(Length Length) {
  void *ptr = MemoryCreate(Length);
  Policy_Append(ptr, MemoryPolicy_Const);
  return ptr;
}