
#include "Chs.h"
#include "GarbageCollection.h"
#include "Private_String.h"

void
String_Append(String* Self, String* Value)
{
  wcs temp = __WcsCreate(Self->Length + Value->Length);
  __WcsWcsSet(temp, Self->Value, Self->Length);

  int i;
  for (i = 0; i < Value->Length; i++)
    temp[Self->Length + i - 1] = Value->Value[i];
  temp[Self->Length + i] = L'\0';
  String_Set(Self, String(temp));
}
