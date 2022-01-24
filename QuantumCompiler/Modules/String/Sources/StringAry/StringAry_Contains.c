
#include "GarbageCollection.h"
#include "Private_StringAry.h"

Length_t
StringAry_Contains(StringAry* Self, String* Value)
{
  return StringAry_Search(Self, Value) != -1;
}