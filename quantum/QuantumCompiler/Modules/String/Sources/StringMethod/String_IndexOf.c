
#include <Private_String.h>

Index_t
String_IndexOf(String pSelf, String pValue)
{
  int i;
  for (i = 0; i < pSelf->m_Length - pValue->m_Length + 1; i++)
    if (pSelf->m_Value[i] == pValue->m_Value[0])
      if (_StringCompare(pSelf->m_Value, pValue, i))
        return i;
  return -1;
}