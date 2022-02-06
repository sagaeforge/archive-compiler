
#include <Private_String.h>

Index_t
String_IndexAt(String pSelf, String pValue, Index_t pStart)
{
  if (pSelf->m_Length <= pStart)
    return -1;

  int i;
  for (i = pStart; i < pSelf->m_Length - pValue->m_Length + 1; i++)
    if (pSelf->m_Value[i] == pValue->m_Value[0])
      if (_StringCompare(pSelf->m_Value, pValue, i))
        return i;
  return -1;
}