
#include <Module/nString.h>

bool
String_IsLower(const nString_t* pSelf)
{
  Index_t _i;
  for (_i = 0; _i < pSelf->m_Length; _i++)
    if (!__IsLower(pSelf->m_Value[_i]))
      return false;
  return true;
}
