
#include <Module/nDigitAry.h>

void
DigitAry_Join(nDigitAry_ptr pSelf, nDigitAry_ptr pTarget)
{
  Index_t _i;
  for (_i = 0; _i < pTarget->m_Length; _i++)
    DigitAry_Push(pSelf, DigitAry_get(pSelf, _i));
}
