
#include <Module/nString.h>
#include <Module/nStringAry.h>

Index_t
StringAry_Search(const nStringAry_t* pSelf, const nString_t* pValue)
{
  Index_t _i;
  for (_i = 0; _i < pSelf->m_Length; _i++)
    if (!String_Compare(StringAry_get(pSelf, _i), pValue))
      return _i;
  return -1;
}
