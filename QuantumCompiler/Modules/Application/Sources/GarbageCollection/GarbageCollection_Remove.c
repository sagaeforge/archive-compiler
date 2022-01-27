
#include <Application.h>
#include <GarbageCollection.h>
#include <Private_GarbageCollection.h>

void
GarbageCollection_Remove(const void* pObj)
{
  MemoryPage page;
  Index_t Index;
  Memory info = GarbageCollection_Find(pObj, &page, &Index);
  if (info == NULL)
    // TODO Exception 처리
    return;

  Excute_MemorySet(&page->Nodes[Index], 0, 1, sizeof(Memory_t));

  int i;
  for (i = Index; i < page->UsedMemoryLength; i++)
    Excute_MemorySwap(&page->Nodes[i], &page->Nodes[i + 1], sizeof(Memory_t));

  page->UsedMemoryLength--;
  Application.Member.GarbageCollection_HeapTable.TotalUsedMemoryLength--;
}
