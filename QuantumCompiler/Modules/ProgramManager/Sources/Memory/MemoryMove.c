
#include "Exception.h"
#include "GarbageCollection.h"
#include "Private_GarbageCollection.h"
#include <stdlib.h>

void MemoryMove(void *Src, void *Data, Length Length) {
  if (GC_CreateCheck(Src, Data))
    return;
  if (Policey(Src, MemoryPolicey_NotMemoryMove) ||
      Policey(Data, MemoryPolicey_NotMemoryMove))
    return;
  if (Policey(Src, MemoryPolicey_Const))
    return;
  if (GC_IndexOfExceptionCheck(Src, Length) ||
      GC_IndexOfExceptionCheck(Data, Length))
    return;

  char *buf = (char *)malloc(Length);
  if (buf == NULL) {
    Warning("버퍼 메모리를 확보할 수 없었습니다.");
    return;
  }

  MemoryCopy(buf, Data, Length);
  MemoryCopy(Src, buf, Length);
  free(buf);
}