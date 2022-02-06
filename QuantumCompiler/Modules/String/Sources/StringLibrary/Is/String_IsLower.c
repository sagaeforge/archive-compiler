
#include <Chs.h>
#include <Private_StringLib.h>

bool
String_IsLower(String pSelf)
{
  int i;
  for (i = 0; i < pSelf->m_Length; i++)
    if (!__IsLower(pSelf->m_Value[i]))
      return false;
  return true;
}