
#include <Exception.h>
#include <GarbageCollection.h>
#include <Private_StringAry.h>

String
StringAry_Pop(StringAry Self)
{
  StringAryNode* node = Self->Values;
  StringAryNode* backup = node;
  if (Self->Length == 0) {
    Exception(ERROR, "가지고 있는 원소가 없습니다.");
    return NULL;
  }

  String temp = NULL;
  if (Self->Length == 1) {
    temp = Self->Values->Value;
    MemoryRemove(Self->Values);
    return temp;
  }

  while (node->Next != NULL) {
    backup = node;
    node = node->Next;
  }
  temp = node->Value;
  MemoryRemove(node);
  backup->Next = NULL;
  Self->Length--;
  return temp;
}