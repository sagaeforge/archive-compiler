
#include <Module/nString.h>

bool
String_IsAlphaDigit(const nString_t* pSelf)
{
  Index_t _i;
  for (_i = 0; _i < pSelf->m_Length; _i++)
    if (!__IsAlphaDigit(pSelf->m_Value[_i]))
      return false;
  return true;
}
