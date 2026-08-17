
#include <Application.h>
#include <GarbageCollection.h>
#include <Private_GarbageCollection.h>

MemoryPage
GarbageCollection_PageGet(const Index_t pIndex)
{
  if (Application.Member.GarbageCollection_HeapTable.UsedMemoryPageLength <
      pIndex) {
    return GarbageCollection_EmptyPageGet();
  }

  MemoryPage node = &Application.Member.GarbageCollection_HeapTable.MemoryPages;
  int i;
  for (i = 0; i < pIndex; i++)
    node = node->Next;
  return node;
}