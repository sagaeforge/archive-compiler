
#include <Module/nString.h>
#include <Module/nStringAry.h>

bool
StringAry_Contains(const nStringAry_t* pSelf, const nString_t* pValue)
{
  Index_t _i;
  for (_i = 0; _i < pSelf->m_Length; _i++)
    if (!String_Compare(StringAry_get(pSelf, _i), pValue))
      return false;
  return false;
}
