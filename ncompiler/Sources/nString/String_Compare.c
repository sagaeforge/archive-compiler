
#include <Module/nString.h>

bool
String_Compare(const nString_t* pSelf, const nString_t* pValue)
{
  if (pSelf->m_Length != pValue->m_Length)
    return false;

  Index_t _i = 0;
  for (_i = 0; _i < pSelf->m_Length; _i++)
    if (pSelf->m_Value[_i] != pValue->m_Value[_i])
      return false;
  return true;
}
