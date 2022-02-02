
#include <Chs.h>
#include <GarbageCollection.h>
#include <Private_String.h>

void
String_Append(String Self, String Value)
{
  if (Value->IsNone)
    return;

  wcs temp = __WcsCreate(Self->Length + Value->Length);
  __WcsWcsInsert(temp, Self->Value, 0, Self->Length);
  __WcsWcsInsert(temp, Value->Value, Self->Length, Value->Length);
  String_Set(Self, String(temp));
}
