
#include <Chs.h>
#include <Private_StringLib.h>

bool
String_IsBinary(String pSelf)
{
  int i;
  if (!(pSelf->Value[0] == '0' &&
        (pSelf->Value[1] == 'b' || pSelf->Value[1] == 'B')))
    return false;
  for (i = 2; i < pSelf->Length; i++)
    if (!__IsBinary(pSelf->Value[i]))
      return false;
  return true;
}