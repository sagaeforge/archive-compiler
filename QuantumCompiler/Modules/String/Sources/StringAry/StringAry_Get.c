
#include <GarbageCollection.h>
#include <Private_StringAry.h>

String
StringAry_Get(StringAry pSelf, Index_t pIndex)
{
  if (pIndex >= pSelf->Length)
    pIndex = pSelf->Length;

  StringAryNode* node = pSelf->Values;
  int i;
  for (i = 0; i < pIndex; i++)
    node = node->Next;
  return node->Value;
}
