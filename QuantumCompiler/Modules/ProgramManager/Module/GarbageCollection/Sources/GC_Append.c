
#include "Private_GarbageCollection.h"

void GC_Append(void *ptr, Length Length) {
  MemoryPage *page = EmptyPageGet();

  page->Datas[page->UsedMemoryLength].Position = ptr;
  page->Datas[page->UsedMemoryLength].Length = Length;
  page->Datas[page->UsedMemoryLength].Policy = MemoryPolicy_None;
  page->UsedMemoryLength++;
  Application.Member.GarbageCollection_UsedMemoryLength++;
}