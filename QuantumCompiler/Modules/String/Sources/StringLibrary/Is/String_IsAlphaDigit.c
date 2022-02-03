
#include <Chs.h>
#include <Private_StringLib.h>

bool
String_IsAlphaDigit(String pSelf)
{
  int i;
  for (i = 0; i < pSelf->Length; i++)
    if (!__IsAlpha(pSelf->Value[i]) || !__IsDecimal(pSelf->Value[i]))
      return false;
  return true;
}
