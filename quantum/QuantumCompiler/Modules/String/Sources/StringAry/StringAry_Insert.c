
#include <GarbageCollection.h>
#include <Private_StringAry.h>

void
StringAry_Insert(StringAry pSelf, String pValue, Index_t pIndex)
{
  if (pIndex >= pSelf->m_Length - 1)
    StringAry_Push(pSelf, pValue);

  StringAryNode* node = StringAry_NodeCreate();
  node->m_Value = pValue;
  node->Next = NULL;
  pSelf->m_Length++;

  StringAryNode* insertNode = pSelf->m_Values;
  if (pIndex == 0) {
    pSelf->m_Values = node;
    node->Next = insertNode;
    return;
  }

  int i;
  for (i = 0; i < pIndex - 1; i++)
    insertNode = insertNode->Next;

  StringAryNode* backup = insertNode;
  insertNode->Next = node;
  node->Next = backup;
}
