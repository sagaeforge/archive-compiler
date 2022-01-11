
#include "Chs.h"
#include "GarbageCollection.h"
#include "Private_String.h"

void
String_Set(String* Self, String* Value)
{
  if (Self->Policy == StringPolicy_Null)
    Self->Value = Value->Value;
  else {
    MemoryRemove((void**)&Self->Value);
    Self->Value = __WcsCreate(Value->Length);
  }
  Self->Policy = Value->Policy;
  Self->Length = Value->Length;

  __StrSet(Self->Value, Value->Value, 4, Value->Length);
}
