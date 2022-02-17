
#include <Module/nString.h>

bool
String_IsHex(const nString_t* pSelf)
{
  Index_t _i;
  if (!(pSelf->m_Value[0] == '0' && __ToUpper(pSelf->m_Value[1]) == 'X'))
    return false;
  for (_i = 2; _i < pSelf->m_Length; _i++)
    if (!__IsHex(pSelf->m_Value[_i]))
      return false;
  return true;
}
