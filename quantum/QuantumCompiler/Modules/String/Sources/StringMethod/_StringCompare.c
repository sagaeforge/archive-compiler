
#include <Private_String.h>

bool
_StringCompare(wcs pAry, String pFindValue, Index_t pStart)
{
  int i;
  for (i = pStart; i < pStart + pFindValue->m_Length; i++)
    if (pAry[i] != pFindValue->m_Value[i - pStart])
      return false;
  return true;
}