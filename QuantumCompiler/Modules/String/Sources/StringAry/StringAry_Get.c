
#include "GarbageCollection.h"
#include "Private_StringAry.h"

String*
StringAry_Get(StringAry* Self, Index_t Index)
{
  if (Index >= Self->Length)
    // TODO Exception 처리
    return NULL;

  StringAryNode* node = Self->Values;
  int i;
  for (i = 0; i < Index; i++)
    node = node->Next;
  return node->Value;
}
