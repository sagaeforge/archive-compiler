
#include <Chs.h>
#include <Private_StringLib.h>

bool
String_IsUpper(String pSelf)
{
  int i;
  for (i = 0; i < pSelf->Length; i++)
    if (!__IsUpper(pSelf->Value[i]))
      return false;
  return true;
}