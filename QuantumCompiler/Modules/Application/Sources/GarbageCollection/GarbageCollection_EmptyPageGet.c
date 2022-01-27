
#include <Application.h>
#include <GarbageCollection.h>
#include <Private_GarbageCollection.h>

#include <stdlib.h>

static MemoryPage
GetPage()
{
  // 페이지 생성 함수
  MemoryPage page = malloc(sizeof(MemoryPage_t));
  if (page == NULL)
    // TODO Exception 처리
    return NULL;
  page->Next = NULL;
  page->UsedMemoryLength = 0;

  int i;
  for (i = 0; i < MemoryMaxLength; i++)
    MemorySet(&page->Nodes[i], 0, 1, sizeof(page->Nodes[0]));

  Application.Member.GarbageCollection_HeapTable.UsedMemoryPageLength++;

  return page;
}

MemoryPage
GarbageCollection_EmptyPageGet()
{
  MemoryPage node = &Application.Member.GarbageCollection_HeapTable.MemoryPages;
  MemoryPage back;
  while (node != NULL) {
    if (node->UsedMemoryLength < MemoryMaxLength)
      return node;
    back = node;
    node = node->Next;
  }

  // 페이지 생성조건
  back->Next = GetPage();
  return back->Next;
}
