

#include "Private_GarbageCollection.h"

void*
MemoryConstCreate(Length Length)
{
  void* ptr = MemoryCreate(Length);
  Policy_Append(ptr, MemoryPolicy_Const);
  return ptr;
}