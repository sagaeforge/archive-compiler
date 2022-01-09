
#include "GarbageCollection.h"
#include "Private_StringAry.h"

Length
StringAry_Contains(StringAry* Self, String* Value)
{
  return StringAry_Search(Self, Value) != -1;
}