
#include "Chs.h"
#include "Private_String.h"

String
String_Loop(String Self, Length_t Length)
{
  wcs temp = __WcsCreate(Self->Length * Length);

  int i, j;
  for (i = 0; i < Length; i++)
    __WcsWcsInsert(temp, Self->Value, Self->Length * i, Self->Length);

  return String(temp);
}