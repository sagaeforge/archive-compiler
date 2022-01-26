
#include "Application.h"
#include "GarbageCollection.h"
#include "Private_GarbageCollection.h"

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
  page->UsedMemoryLength++;
  Application.Member.GarbageCollection_HeapTable.TotalUsedMemoryLength++;
}