
#include <GarbageCollection.h>
#include <Private_GarbageCollection.h>

void
Excute_MemorySwap(void* pObj, void* pData, const Length_t pLength)
{
  char* a = (char*)pObj;
  char* b = (char*)pData;

  int i = 0;
  char t = '\0';
  while (i < pLength) {
    t = *a;
    *a = *b;
    *b = t;
    i++, a++, b++;
  }
}

void
MemorySwap(void* pObj, void* pData, const Length_t pLength)
{
  Excute_MemorySwap(pObj, pData, pLength);
}