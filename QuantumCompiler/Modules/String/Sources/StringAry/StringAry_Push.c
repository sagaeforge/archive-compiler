
#include <GarbageCollection.h>
#include <Private_StringAry.h>

void
StringAry_Push(StringAry pSelf, String pValue)
{
  StringAryNode* node = StringAry_NodeCreate();
  node->Next = NULL;
  node->Value = pValue;

  if (pSelf->Values == NULL) {
    pSelf->Values = node;
    pSelf->Length++;
    return;
  }

  StringAryNode* InsertNode = pSelf->Values;
  while (InsertNode->Next != NULL)
    InsertNode = InsertNode->Next;

  InsertNode->Next = node;
  pSelf->Length++;
}