
#include "Chs.h"
#include "Private_String.h"

// TODO 오류 검사
String
String_Join(String Self, String Value)
{
  wcs temp = __WcsCreate(Self->Length + Value->Length);

  __WcsWcsInsert(temp, Self->Value, 0, Self->Length);
  __WcsWcsInsert(temp, Value->Value, Self->Length - 1, Value->Length);

  return String(temp);
}
