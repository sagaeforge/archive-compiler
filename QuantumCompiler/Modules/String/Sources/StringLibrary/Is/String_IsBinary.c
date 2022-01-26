
#include "Chs.h"
#include "Private_StringLib.h"

bool
String_IsBinary(String Self)
{
  int i;
  if (!(Self->Value[0] == '0' &&
        (Self->Value[1] == 'b' || Self->Value[1] == 'B')))
    return false;
  for (i = 2; i < Self->Length; i++)
    if (!__IsBinary(Self->Value[i]))
      return false;
  return true;
}