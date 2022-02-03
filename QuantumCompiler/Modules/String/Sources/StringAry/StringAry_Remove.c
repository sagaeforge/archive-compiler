
#include <GarbageCollection.h>
#include <Private_StringAry.h>

void
StringAry_Remove(StringAry pSelf, Index_t pIndex)
{
  pIndex = pIndex >= pSelf->Length ? pSelf->Length : pIndex;

  StringAryNode* node = pSelf->Values;
  StringAryNode* backup = node;
  pSelf->Length--;
  if (pIndex == 0) {
    node = node->Next;
    pSelf->Values = node;
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