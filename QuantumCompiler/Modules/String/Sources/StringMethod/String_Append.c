
#include <Chs.h>
#include <GarbageCollection.h>
#include <Private_String.h>

void
String_Append(String Self, String Value)
{
  wcs temp = __WcsCreate(Self->Length + Value->Length);
  __WcsWcsInsert(temp, Self->Value, 0, Self->Length);

  if (!Self->IsNone) {
    MemoryRemove(Self->Value);
  }

  __WcsWcsInsert(temp, Value->Value, Self->Length - 1, Value->Length);
  String_Set(Self, String(temp));
}
