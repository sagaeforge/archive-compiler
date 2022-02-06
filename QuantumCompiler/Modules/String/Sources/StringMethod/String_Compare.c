
#include <Private_String.h>

bool
String_Compare(String pSelf, String pValue)
{
  if (pSelf->m_Length != pValue->m_Length)
    return false;

  int i;
  for (i = 0; i < pSelf->m_Length; i++)
    if (pSelf->m_Value[i] != pValue->m_Value[i])
      return false;
  return true;
}
