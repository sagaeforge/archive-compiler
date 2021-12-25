
#include "Exception.h"
#include "GarbageCollection.h"
#include "Private_GarbageCollection.h"
#include "ProgramManager.h"
#include <stdlib.h>

void MemorySet(void *Src, int value, Length WordSize, Length Length) {
  MemoryInfo info = Info(Src);
  if (!info.IsFounded)
    Warning("GC에서 생성된 메모리가 아닙니다. --> %p", info.Value);
  if (Policey(Src, MemoryPolicey_NotMemorySet))
    return;
  if (Policey(Src, MemoryPolicey_Const))
    return;
  if (GC_IndexOfExceptionCheck(Src, Length))
    return;
  if (WordSize == 0 || WordSize == 3 || WordSize > 4)
    return;

  Length *= WordSize;
  char *a = (char *)Src;
  char *b = (char *)&value;
  const char *backup = b;
  int i = 0, j = 0;
  while (i < Length) {
    while (j < WordSize) {
      *a = *b;
      a++, b++, i++, j++;
    }
    j = 0;
    b = (char *)backup;
  }
}
