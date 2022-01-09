
#include "GarbageCollection.h"
#include "Private_StringAry.h"

void
StringAry_Push(StringAry* Self, String* Value)
{
  StringAryNode* node = StringAry_NodeCreate();
  node->Next = NULL;
  node->Value = Value;

  if (Self->Values == NULL) {
    Self->Values = node;
    Self->Length++;
    return;
  }

  StringAryNode* InsertNode = Self->Values;
  while (InsertNode->Next != NULL)
    InsertNode = InsertNode->Next;

  InsertNode->Next = node;
  Self->Length++;
}