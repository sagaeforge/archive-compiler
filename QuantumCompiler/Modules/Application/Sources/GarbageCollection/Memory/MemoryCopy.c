
#include "GarbageCollection.h"
#include "Private_GarbageCollection.h"

void
Excute_MemoryCopy(void* pObj, const void* pData, const Length_t pLength)
{
  char* a = (char*)pObj;
  char* b = (char*)pData;

  int i = 0;
  while (i < pLength) {
    *a = *b;
    i++, a++, b++;
  }
}

void
MemoryCopy(void* pObj, const void* pData, const Length_t pLength)
{}