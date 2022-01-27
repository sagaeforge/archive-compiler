
#include <Chs.h>
#include <Private_StringLib.h>

bool
String_IsLower(String Self)
{
  int i;
  for (i = 0; i < Self->Length; i++)
    if (!__IsLower(Self->Value[i]))
      return false;
  return true;
}