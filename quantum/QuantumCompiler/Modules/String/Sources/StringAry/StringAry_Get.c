
#include <GarbageCollection.h>
#include <Private_StringAry.h>

String
StringAry_Get(StringAry pSelf, Index_t pIndex)
{
  if (pIndex >= pSelf->m_Length)
    pIndex = pSelf->m_Length - 1;

  StringAryNode* node = pSelf->m_Values;
  int i;
  for (i = 0; i < pIndex; i++)
    node = node->Next;
  return node->m_Value;
}
