
#include <Chs.h>
#include <Private_StringLib.h>

bool
String_IsHex(String Self)
{
  int i;
  if (!(Self->Value[0] == '0' &&
        (Self->Value[1] == 'x' || Self->Value[1] == 'X')))
    return false;
  for (i = 2; i < Self->Length; i++)
    if (!__IsHex(Self->Value[i]))
      return false;
  return true;
}