
#include <Chs.h>
#include <Private_StringLib.h>

bool
String_IsControl(String pSelf)
{
  int i;
  for (i = 0; i < pSelf->Length; i++)
    if (!__IsControl(pSelf->Value[i]))
      return false;
  return true;
}
