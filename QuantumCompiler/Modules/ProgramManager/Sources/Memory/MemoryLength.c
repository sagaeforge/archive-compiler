
#include "Exception.h"
#include "GarbageCollection.h"
#include "Private_GarbageCollection.h"
#include "ProgramManager.h"
#include <stdlib.h>

Length MemoryLength(void *Obj) {
  MemoryInfo info = Info(Obj);
  if (!info.IsFounded) {
    Warning("GC에서 생성된 메모리가 아닙니다. --> %p", info.Value);
    return 0;
  }
  if (Policey(Obj, MemoryPolicey_NotMemoryLength))
    return 0;

  return info.Length;
}