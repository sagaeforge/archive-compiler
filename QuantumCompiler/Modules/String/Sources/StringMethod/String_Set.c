
#include <Chs.h>
#include <GarbageCollection.h>
#include <Private_String.h>

void
String_Set(String Self, String Value)
{
  if (Self->IsNone)
    Self->Value = Value->Value;
  else {
    MemoryRemove(Self->Value);
    wcs temp = __WcsCreate(Value->Length);
    __StrSet(temp, Value->Value, 4, Value->Length);
    Self->Value = temp;
  }

  Self->IsNone = Value->IsNone;
  Self->Length = Value->Length;
}
