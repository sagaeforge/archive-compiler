
#include "GarbageCollection.h"
#include "Application.h"
#include "Private_GarbageCollection.h"

#include <stdlib.h>

static void
GarbageCollectionModule_ObjectTableInitialized()
{
  // Object 테이블 풀링 적용
  Application.Member.GarbageCollection_ObjectTable.UsedObjectLength = 0;
  int i;
  for (i = 0; i < ObjectMaxLength; i++) {
    Application.Member.GarbageCollection_ObjectTable.Value[i] =
      (Object)malloc(sizeof(Object_t));
    Application.Member.GarbageCollection_ObjectTable.Value[i]->m_Info = NULL;
    Application.Member.GarbageCollection_ObjectTable.Value[i]
      ->m_Value.m_Value1d = NULL;
    Application.Member.GarbageCollection_ObjectTable.IsUsed[i] = false;
  }
}

static void
GarbageCollectionModule_HeapTableInitialized()
{
  Application.Member.GarbageCollection_HeapTable.UsedMemoryPageLength = 1;
  Application.Member.GarbageCollection_HeapTable.MemoryPages.Next = NULL;
  Application.Member.GarbageCollection_HeapTable.MemoryPages.UsedMemoryLength =
    0;
  Application.Member.GarbageCollection_HeapTable.UsedMemoryLength = 0;

  int i;
  for (i = 0; i < MemoryMaxLength; i++) {
    Application.Member.GarbageCollection_HeapTable.MemoryPages.Nodes[i]
      .m_Length = 0;
    Application.Member.GarbageCollection_HeapTable.MemoryPages.Nodes[i]
      .m_Policy = MemoryPolicy_None;
    Application.Member.GarbageCollection_HeapTable.MemoryPages.Nodes[i]
      .m_TypeInfo = NULL;
    Application.Member.GarbageCollection_HeapTable.MemoryPages.Nodes[i]
      .m_Value = NULL;
  }
}

void
GarbageCollectionModule_Initialized()
{
  GarbageCollectionModule_HeapTableInitialized();
  GarbageCollectionModule_ObjectTableInitialized();
}