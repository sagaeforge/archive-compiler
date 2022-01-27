
#include <GarbageCollection.h>
#include <Private_GarbageCollection.h>

bool
Excute_MemoryCompare(const void* pObj1,
                     const void* pObj2,
                     const Length_t pLength)
{
  char* a = (char*)pObj1;
  char* b = (char*)pObj2;
  int i;
  for (i = 0; i < pLength; a++, b++, i++)
    if (*a != *b)
      return false;
  return true;
}

bool
MemoryCompare(const void* pObj1, const void* pObj2, const Length_t pLength)
{
  return Excute_MemoryCompare(pObj1, pObj2, pLength);
}
