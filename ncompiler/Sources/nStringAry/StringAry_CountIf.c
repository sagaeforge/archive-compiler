
#include <Module/nStringAry.h>

Length_t
StringAry_CountIf(const nStringAry_t* pSelf, bool (*pExpression)(nString_t*, void*), void* pParam)
{
  Index_t _i, _c = 0;
  for (_i = 0; _i < pSelf->m_Length; _i++)
    if (pExpression(StringAry_get(pSelf, _i), pParam))
      _c++;
  return _c;
}
