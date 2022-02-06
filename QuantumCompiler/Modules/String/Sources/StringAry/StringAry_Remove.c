
#include <GarbageCollection.h>
#include <Private_StringAry.h>

void
StringAry_Remove(StringAry pSelf, Index_t pIndex)
{
  pIndex = pIndex >= pSelf->m_Length ? pSelf->m_Length : pIndex;

  StringAryNode* node = pSelf->m_Values;
  StringAryNode* backup = node;
  pSelf->m_Length--;
  if (pIndex == 0) {
    node = node->Next;
    pSelf->m_Values = node;
    MemoryRemove(backup);
    return;
  }

  int i;
  for (i = 0; i < pIndex - 1; i++) {
    backup = node;
    node = node->Next;
  }
  node->Next = backup->Next;
  MemoryRemove(backup);
}