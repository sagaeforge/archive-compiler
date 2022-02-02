
#include <Application.h>
#include <Exception.h>
#include <GarbageCollection.h>
#include <Private_GarbageCollection.h>

const Memory
GarbageCollection_Find(const void* pObj,
                       MemoryPage* Out_pMemoryPage,
                       Index_t* Out_pIndex)
{
  MemoryPage page = &Application.Member.GarbageCollection_HeapTable.MemoryPages;

  if (pObj == NULL) {
    Exception(ERROR, "해당 메모리는 생성된 메모리가 아닙니다. [NULL 포인터]");
    return NULL;
  }

  int i;
  for (i = 0; page != NULL; i++) {
    int pl = 0;
    int pr = page->UsedMemoryLength;
    int pc = 0;

    while (pl <= pr) {
      pc = (pl + pr) / 2;

      if (page->Nodes[pc].m_Value == pObj) {
        *Out_pMemoryPage = page;
        *Out_pIndex = pc;
        return &page->Nodes[i];
      } else if (page->Nodes[pc].m_Value < pObj)
        pl = pc + 1;
      else
        pr = pc - 1;
    }
    page = page->Next;
  }

  return NULL;
}
