
#include "GarbageCollection.h"
#include "Private_GarbageCollection.h"

void
Excute_MemorySet(void* pObj,
                 const int pValue,
                 const Length_t pWordSize,
                 const Length_t pLength)
{
  char* a = (char*)pObj;
  char* b = (char*)&pValue;
  const char* backup = b;
  const Length_t TotalLen = pLength * pWordSize;

  int i = 0;
  while (i < TotalLen) {
    int j;
    for (j = 0; j < pWordSize; a++, b++, i++, j++)
      *a = *b;
    b = (char*)backup;
  }
}

void
MemorySet(void* pObj,
          const int pValue,
          const Length_t pWordSize,
          const Length_t pLength)
{}