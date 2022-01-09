
#include "Exception.h"
#include "GarbageCollection.h"
#include "Private_GarbageCollection.h"
#include "ProgramManager.h"
#include <stdlib.h>

Length
MemoryLength(void* Obj)
{
  MemoryInfo info = Info(Obj);
  return info.Memory.Length;
}