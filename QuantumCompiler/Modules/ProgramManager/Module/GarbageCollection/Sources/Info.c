
#include "Private_GarbageCollection.h"
#include "ProgramManager.h"

MemoryInfo
Info(void* Obj)
{
  MemoryPage* page = &Application.Member.GarbageCollection_Pages;
  MemoryInfo ret = {
    0,
  };

  if (Obj == NULL)
    // TODO Exception 처리
    return ret;

  int i;
  for (i = 0; page != NULL; i++) {
    if (Obj == page) {
      ret = page->Info;
      ret.IsFound = true;
      return ret;
    }

    int pl = 0;
    int pr = page->UsedMemoryLength;
    int pc = 0;

    do {
      pc = (pl + pr) / 2;

      if (page->Datas[pc].Position == Obj) {
        ret.IsFound = true;
        ret.Memory = page->Datas[pc];
        ret.Position.MemoryIndex = pc;
        ret.Position.PageIndex = i;
        return ret;
      } else if (page->Datas[pc].Position < Obj)
        pl = pc + 1;
      else
        pr = pc - 1;

    } while (pl <= pr);
    page = page->Next;
  }

  return ret;
}