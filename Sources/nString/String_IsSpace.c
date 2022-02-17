
#include <Module/nString.h>

bool
String_IsSpace(const nString_t* pSelf)
{
  Index_t _i;
  for (_i = 0; _i < pSelf->m_Length; _i++)
    if (!__IsSpace(pSelf->m_Value[_i]))
      return false;
  return true;
}
