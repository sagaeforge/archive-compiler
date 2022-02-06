
#include <Chs.h>
#include <Private_StringLib.h>

bool
String_IsOctal(String pSelf)
{
  int i;
  if (!(pSelf->m_Value[0] == '0' &&
        (pSelf->m_Value[1] == 'o' || pSelf->m_Value[1] == 'O')))
    return false;
  for (i = 2; i < pSelf->m_Length; i++)
    if (!__IsOctal(pSelf->m_Value[i]))
      return false;
  return true;
}