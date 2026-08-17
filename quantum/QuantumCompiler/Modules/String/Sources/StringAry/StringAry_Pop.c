
#include <Exception.h>
#include <GarbageCollection.h>
#include <Private_StringAry.h>

String
StringAry_Pop(StringAry pSelf)
{
  StringAryNode* node = pSelf->m_Values;
  StringAryNode* backup = node;
  if (pSelf->m_Length == 0) {
    Exception(ERROR, "가지고 있는 원소가 없습니다.");
    return NULL;
  }

  String temp = NULL;
  if (pSelf->m_Length == 1) {
    temp = pSelf->m_Values->m_Value;
    MemoryRemove(pSelf->m_Values);
    return temp;
  }

  while (node->Next != NULL) {
    backup = node;
    node = node->Next;
  }
  temp = node->m_Value;
  MemoryRemove(node);
  backup->Next = NULL;
  pSelf->m_Length--;
  return temp;
}