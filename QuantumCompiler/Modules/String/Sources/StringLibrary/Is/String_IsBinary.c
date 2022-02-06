
#include <Chs.h>
#include <Private_StringLib.h>

bool
String_IsBinary(String pSelf)
{
  int i;
  if (!(pSelf->m_Value[0] == '0' &&
        (pSelf->m_Value[1] == 'b' || pSelf->m_Value[1] == 'B')))
    return false;
  for (i = 2; i < pSelf->m_Length; i++)
    if (!__IsBinary(pSelf->m_Value[i]))
      return false;
  return true;
}