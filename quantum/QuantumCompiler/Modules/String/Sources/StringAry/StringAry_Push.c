
#include <GarbageCollection.h>
#include <Private_StringAry.h>

void
StringAry_Push(StringAry pSelf, String pValue)
{
  StringAryNode* node = StringAry_NodeCreate();
  node->Next = NULL;
  node->m_Value = pValue;

  if (pSelf->m_Values == NULL) {
    pSelf->m_Values = node;
    pSelf->m_Length++;
    return;
  }

  StringAryNode* InsertNode = pSelf->m_Values;
  while (InsertNode->Next != NULL)
    InsertNode = InsertNode->Next;

  InsertNode->Next = node;
  pSelf->m_Length++;
}