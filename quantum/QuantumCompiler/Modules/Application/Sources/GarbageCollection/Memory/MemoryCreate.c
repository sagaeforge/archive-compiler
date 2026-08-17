
#include <Exception.h>
#include <GarbageCollection.h>
#include <Private_GarbageCollection.h>

#include <stdlib.h>

__attribute__((warn_unused_result)) void*
Excute_MemoryCreate(const Length_t pLength)
{
  void* ptr = malloc(pLength);
  if (ptr == NULL) {
    Exception(ERROR, "메모리 할당에 실패했습니다. [size:%u]", pLength);
    return NULL;
  }

  GarbageCollection_Append(ptr, NULL, pLength);
  Excute_MemorySet(ptr, 0, 1, pLength);
  return ptr;
}

__attribute__((warn_unused_result)) void*
MemoryCreate(const Length_t pLength)
{
  return Excute_MemoryCreate(pLength);
}