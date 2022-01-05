
#include "GarbageCollection.h"
#include "Private_GarbageCollection.h"
#include "ProgramManager.h"
#include <stdlib.h>

MemoryPage *EmptyPageGet() {
  MemoryPage *Pos = &Application.Member.GarbageCollection_Pages;
  MemoryPage *Last = Pos;
  while (Pos != NULL)
    if (Pos->UsedMemoryLength < MemoryMaxLength)
      return Pos;
    else {
      if (Pos->Next == NULL) {
        Last = Pos;
        break;
      } else
        Pos = Pos->Next;
    }

  // 메모리 페이지 생성 및 초기화
  MemoryPage *page = (MemoryPage *)malloc(sizeof(MemoryPage));
  MemorySet(page->Datas, 0, 1, sizeof(page->Datas[0]) * MemoryMaxLength);
  page->UsedMemoryLength = 0;
  page->Next = NULL;
  page->Info.IsFound = false;
  page->Info.Memory.Length = sizeof(MemoryPage);
  page->Info.Memory.Policy = MemoryPolicy_None;
  page->Info.Memory.Position = page;
  page->Info.Position.MemoryIndex = 0;
  page->Info.Position.PageIndex =
      Application.Member.GarbageCollection_UsedMemoryPageLength;

  Last->Next = page;
  Application.Member.GarbageCollection_UsedMemoryPageLength++;
  return page;
}

MemoryPage *PageGet(Index Index) {
  if (Index >= Application.Member.GarbageCollection_UsedMemoryPageLength)
    return EmptyPageGet();

  int i;
  MemoryPage *Pos = &Application.Member.GarbageCollection_Pages;
  for (i = 0; i < Index; i++) {
    Pos = Pos->Next;
  }
  return Pos;
}