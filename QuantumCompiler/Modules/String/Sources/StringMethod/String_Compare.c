
#include "GarbageCollection.h"
#include "Private_String.h"

bool String_Compare(String *Self, String *Value) {
  if (Self->Length != Value->Length)
    return false;
  return MemoryCompare(Self->Value, Value->Value, Self->Length);
}
