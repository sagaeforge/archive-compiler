
#include <Chs.h>
#include <Private_StringLib.h>

bool
String_IsHex(String pSelf)
{
  int i;
  if (!(pSelf->Value[0] == '0' &&
        (pSelf->Value[1] == 'x' || pSelf->Value[1] == 'X')))
    return false;
  for (i = 2; i < pSelf->Length; i++)
    if (!__IsHex(pSelf->Value[i]))
      return false;
  return true;
}