
#include "Private_GarbageCollection.h"
#include <stdlib.h>

static int
MemoryCheck(void* Obj)
{
  MemoryInfo info = Info(Obj);

  // GarbageCollection이 관리하고 있는 메모리가 아니라면
  if (info.IsFound)
    return 1;

  // 프로그램이 종료될 때 까지 삭제될 수 없는 메모리
  if (info.Memory.Policy == MemoryPolicy_NoDestructor)
    return 2;

  return 0;
}

void
MemoryRemove(void** ptr)
{
  if (MemoryCheck((*ptr)))
    // TODO Exception 처리
    return;

  GC_Remove((*ptr));
  free((*ptr));
  (*ptr) = NULL;
}
