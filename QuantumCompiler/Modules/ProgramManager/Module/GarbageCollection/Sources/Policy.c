
#include "GarbageCollection.h"
#include "Private_GarbageCollection.h"
#include "ProgramManager.h"

bool
Policy(void* Obj, MemoryPolicy Policy)
{
  MemoryInfo info = Application.GarbageCollection.Info(Obj);
  return info.Memory.Policy & Policy;
}

void
Policy_Append(void* Obj, MemoryPolicy Policy)
{
  MemoryInfo info = Application.GarbageCollection.Info(Obj);
  MemoryPage* page = PageGet(info.Position.PageIndex);
  page->Datas[info.Position.MemoryIndex].Policy |= Policy;
}

void
Policy_Remove(void* Obj, MemoryPolicy Policy)
{
  MemoryInfo info = Application.GarbageCollection.Info(Obj);
  MemoryPage* page = PageGet(info.Position.PageIndex);
  page->Datas[info.Position.MemoryIndex].Policy &= ~Policy;
}
