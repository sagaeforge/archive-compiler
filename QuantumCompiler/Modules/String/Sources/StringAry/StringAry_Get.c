
#include <GarbageCollection.h>
#include <Private_StringAry.h>

String
StringAry_Get(StringAry Self, Index_t Index)
{
  if (Index >= Self->Length)
    Index = Self->Length;

  StringAryNode* node = Self->Values;
  int i;
  for (i = 0; i < Index; i++)
    node = node->Next;
  return node->Value;
}
