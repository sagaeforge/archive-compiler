
#include "GarbageCollection.h"
#include "Private_StringAry.h"

void
StringAry_Remove(StringAry* Self, Index_t Index)
{
  Index = Index >= Self->Length ? Self->Length : Index;

  StringAryNode* node = Self->Values;
  StringAryNode* backup = node;
  Self->Length--;
  if (Index == 0) {
    node = node->Next;
    Self->Values = node;
    MemoryRemove((void**)&backup);
    return;
  }

  int i;
  for (i = 0; i < Index - 1; i++) {
    backup = node;
    node = node->Next;
  }
  node->Next = backup->Next;
  MemoryRemove((void**)&backup);
}