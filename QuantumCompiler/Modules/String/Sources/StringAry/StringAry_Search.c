
#include <GarbageCollection.h>
#include <Private_String.h>
#include <Private_StringAry.h>

Index_t
StringAry_Search(StringAry pSelf, String pValue)
{
  StringAryNode* node = pSelf->Values;
  int i;
  for (i = 0; node != NULL; i++)
    if (String_Compare(node->Value, pValue))
      return i;
    else
      node = node->Next;
  return -1;
}
