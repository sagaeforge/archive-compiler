
#include <Chs.h>
#include <Private_StringLib.h>

bool
String_IsSpace(String pSelf)
{
  int i;
  for (i = 0; i < pSelf->Length; i++)
    if (!__IsSpace(pSelf->Value[i]))
      return false;
  return true;
}