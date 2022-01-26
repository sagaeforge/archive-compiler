
#include "Chs.h"
#include "Private_StringLib.h"

bool
String_IsDecimal(String Self)
{
  int i;
  for (i = 0; i < Self->Length; i++)
    if (!__IsDecimal(Self->Value[i]))
      return false;
  return true;
}