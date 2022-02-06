
#include <Chs.h>
#include <Private_StringLib.h>

bool
String_IsAlpha(String pSelf)
{
  int i;
  for (i = 0; i < pSelf->m_Length; i++)
    if (!__IsAlpha(pSelf->m_Value[i]))
      return false;
  return true;
}
