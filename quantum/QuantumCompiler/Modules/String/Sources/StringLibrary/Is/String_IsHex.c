
#include <Chs.h>
#include <Private_StringLib.h>

bool
String_IsHex(String pSelf)
{
  int i;
  if (!(pSelf->m_Value[0] == '0' &&
        (pSelf->m_Value[1] == 'x' || pSelf->m_Value[1] == 'X')))
    return false;
  for (i = 2; i < pSelf->m_Length; i++)
    if (!__IsHex(pSelf->m_Value[i]))
      return false;
  return true;
}