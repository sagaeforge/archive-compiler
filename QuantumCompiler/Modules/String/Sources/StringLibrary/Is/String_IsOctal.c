
#include "Chs.h"
#include "Private_StringLib.h"

bool
String_IsOctal(String* Self)
{
  int i;
  if (!(Self->Value[0] == '0' &&
        (Self->Value[1] == 'o' || Self->Value[1] == 'O')))
    return false;
  for (i = 2; i < Self->Length; i++)
    if (!__IsOctal(Self->Value[i]))
      return false;
  return true;
}