
#include <Application.h>
#include <GarbageCollection.h>
#include <Private_GarbageCollection.h>

#include <stdlib.h>

static void
GarbageCollectionModule_ObjectTableInitialized()
{
  // Object 테이블 풀링 적용
  Application.Member.GarbageCollection_ObjectTable.UsedObjectLength = 0;
  int i;
  for (i = 0; i < ObjectMaxLength; i++) {
    Application.Member.GarbageCollection_ObjectTable.Value[i] =
      malloc(sizeof(Object_t));
    Application.Member.GarbageCollection_ObjectTable.IsUsed[i] = false;
    Excute_MemorySet(Application.Member.GarbageCollection_ObjectTable.Value[i],
                     0,
                     1,
                     sizeof(Object_t));
  }
}

static void
GarbageCollectionModule_HeapTableInitialized()
{
  Application.Member.GarbageCollection_HeapTable.UsedMemoryPageLength = 1;
  Application.Member.GarbageCollection_HeapTable.MemoryPages.Next = NULL;
  Application.Member.GarbageCollection_HeapTable.MemoryPages.UsedMemoryLength =
    0;
  Application.Member.GarbageCollection_HeapTable.TotalUsedMemoryLength = 0;

  MemoryPage page = &Application.Member.GarbageCollection_HeapTable.MemoryPages;
  int i;
  for (i = 0; i < MemoryMaxLength; i++) {
    Excute_MemorySet(&page->Nodes[i], 0, 1, sizeof(Memory_t));
  }
}

void
GarbageCollectionModule_Initialized()
{
  GarbageCollectionModule_ObjectTableInitialized();
  GarbageCollectionModule_HeapTableInitialized();
}