
#include "Chs.h"
#include "GarbageCollection.h"
#include "Private_String.h"

void
String_Set(String* Self, String* Value)
{
  if (Self->IsNone == true)
    Self->Value = Value->Value;
  else {
    MemoryRemove((void**)&Self->Value);
    Self->Value = __WcsCreate(Value->Length);
  }
  Self->IsConst = Value->IsConst;
  Self->IsNone = Value->IsNone;
  Self->Length = Value->Length;

  __WcsWcsSet(Self->Value, Value->Value, Value->Length);
}
