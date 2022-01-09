
#include "GarbageCollection.h"
#include "Private_StringAry.h"

void
StringAry_Insert(StringAry* Self, String* Value, Index Index)
{
  if (Index >= Self->Length - 1)
    StringAry_Push(Self, Value);

  StringAryNode* node = StringAry_NodeCreate();
  node->Value = Value;
  node->Next = NULL;
  Self->Length++;

  StringAryNode* insertNode = Self->Values;
  if (Index == 0) {
    Self->Values = node;
    node->Next = insertNode;
    return;
  }

  int i;
  for (i = 0; i < Index - 1; i++)
    insertNode = insertNode->Next;

  StringAryNode* backup = insertNode;
  insertNode->Next = node;
  node->Next = backup;
}
