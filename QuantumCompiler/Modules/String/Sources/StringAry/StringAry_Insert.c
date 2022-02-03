
#include <GarbageCollection.h>
#include <Private_StringAry.h>

void
StringAry_Insert(StringAry pSelf, String pValue, Index_t pIndex)
{
  if (pIndex >= pSelf->Length - 1)
    StringAry_Push(pSelf, pValue);

  StringAryNode* node = StringAry_NodeCreate();
  node->Value = pValue;
  node->Next = NULL;
  pSelf->Length++;

  StringAryNode* insertNode = pSelf->Values;
  if (pIndex == 0) {
    pSelf->Values = node;
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
