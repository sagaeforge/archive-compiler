
#include <Chs.h>
#include <Private_StringLib.h>

bool
String_IsOctal(String pSelf)
{
  int i;
  if (!(pSelf->Value[0] == '0' &&
        (pSelf->Value[1] == 'o' || pSelf->Value[1] == 'O')))
    return false;
  for (i = 2; i < pSelf->Length; i++)
    if (!__IsOctal(pSelf->Value[i]))
      return false;
  return true;
}