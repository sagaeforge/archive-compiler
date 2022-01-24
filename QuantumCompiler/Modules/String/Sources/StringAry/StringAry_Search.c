
#include "GarbageCollection.h"
#include "Private_String.h"
#include "Private_StringAry.h"

Index_t
StringAry_Search(StringAry* Self, String* Value)
{
  StringAryNode* node = Self->Values;
  int i;
  for (i = 0; node != NULL; i++)
    if (String_Compare(node->Value, Value))
      return i;
    else
      node = node->Next;
  return -1;
}
