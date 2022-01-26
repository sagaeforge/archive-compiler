
#include "Chs.h"
#include "Private_StringLib.h"

bool
String_IsAlpha(String Self)
{
  int i;
  for (i = 0; i < Self->Length; i++)
    if (!__IsAlpha(Self->Value[i]))
      return false;
  return true;
}
