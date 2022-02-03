
#include <Exception.h>
#include <GarbageCollection.h>
#include <Private_StringAry.h>

String
StringAry_Pop(StringAry pSelf)
{
  StringAryNode* node = pSelf->Values;
  StringAryNode* backup = node;
  if (pSelf->Length == 0) {
    Exception(ERROR, "가지고 있는 원소가 없습니다.");
    return NULL;
  }

  String temp = NULL;
  if (pSelf->Length == 1) {
    temp = pSelf->Values->Value;
    MemoryRemove(pSelf->Values);
    return temp;
  }

  while (node->Next != NULL) {
    backup = node;
    node = node->Next;
  }
  temp = node->Value;
  MemoryRemove(node);
  backup->Next = NULL;
  pSelf->Length--;
  return temp;
}