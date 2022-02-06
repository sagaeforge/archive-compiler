
#include <GarbageCollection.h>
#include <Private_StringAry.h>

void
StringAry_Destructor(StringAry* pSelf)
{
  StringAry Ary = *pSelf;
  if (Ary->m_Length != 0) {
    StringAryNode** ptr =
      (StringAryNode**)MemoryCreate(sizeof(StringAryNode*) * Ary->m_Length);

    StringAryNode* node = Ary->m_Values;
    int i;
    for (i = 0; node != NULL; i++) {
      ptr[i] = node;
      node = node->Next;
    }
    for (i = 0; i < Ary->m_Length; i++)
      MemoryRemove(ptr[i]);
    MemoryRemove(ptr);
  }

  (*pSelf)->m_Values = NULL;
  (*pSelf)->m_Length = 0;

  MemoryRemove(pSelf);
  (*pSelf) = NULL;
}
