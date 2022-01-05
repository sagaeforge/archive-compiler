
#include "GarbageCollection.h"
#include "Private_GarbageCollection.h"

void GC_Remove(void *ptr) {
  MemoryInfo info = Info(ptr);
  if (!info.IsFound)
    return;

  MemoryPage *page = PageGet(info.Position.PageIndex);
  page->Datas[info.Position.MemoryIndex].Position = NULL;
  page->Datas[info.Position.MemoryIndex].Length = 0;
  page->Datas[info.Position.MemoryIndex].Policy = MemoryPolicy_None;

  page->UsedMemoryLength--;
  Application.Member.GarbageCollection_UsedMemoryLength--;

  int i;
  for (i = info.Position.MemoryIndex; i < page->UsedMemoryLength; i++) {
    MemorySwap(&page->Datas[i], &page->Datas[i + 1], sizeof(page->Datas[i]));
  }
}