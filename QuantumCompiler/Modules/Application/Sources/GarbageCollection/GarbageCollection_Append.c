
#include <Application.h>
#include <GarbageCollection.h>
#include <Private_GarbageCollection.h>

void
GarbageCollection_Append(const void* pObj,
                         const DataTypeInfo_t* pInfo,
                         const Length_t pLength)
{
  if (pObj == NULL)
    return;

  MemoryPage page = GarbageCollection_EmptyPageGet();
  page->Nodes[page->UsedMemoryLength].m_Length = pLength;
  page->Nodes[page->UsedMemoryLength].m_Policy = MemoryPolicy_None;
  page->Nodes[page->UsedMemoryLength].m_TypeInfo = (DataTypeInfo_t*)pInfo;
  page->Nodes[page->UsedMemoryLength].m_Value = (void*)pObj;

  int i;
  for (i = Application.Member.GarbageCollection_HeapTable.TotalUsedMemoryLength;
       i > 0;
       i--) {
    if (page->Nodes[i - 1].m_Value > page->Nodes[i].m_Value)
      Excute_MemorySwap(&page->Nodes[i - 1], &page->Nodes[i], sizeof(Memory_t));
  }

  page->UsedMemoryLength++;
  Application.Member.GarbageCollection_HeapTable.TotalUsedMemoryLength++;
}