
#include <Exception.h>
#include <GarbageCollection.h>
#include <Private_GarbageCollection.h>

#include <stdlib.h>

void
Excute_MemoryMove(void* pObj, const void* pData, const Length_t pLength)
{
  char* buf = (char*)malloc(pLength);
  if (buf == NULL) {
    Exception(ERROR, "임시 공간 할당에 실패했습니다. [size:%u]", pLength);
    return;
  }

  Excute_MemoryCopy(buf, pData, pLength);
  Excute_MemoryCopy(pObj, buf, pLength);
  free(buf);
}

void
MemoryMove(void* pObj, const void* pData, const Length_t pLength)
{
  Excute_MemoryMove(pObj, pData, pLength);
}