
#include <Chs.h>
#include <Private_StringLib.h>

bool
String_IsDecimal(String pSelf)
{
  int i;
  for (i = 0; i < pSelf->m_Length; i++)
    if (!__IsDecimal(pSelf->m_Value[i]))
      return false;
  return true;
}