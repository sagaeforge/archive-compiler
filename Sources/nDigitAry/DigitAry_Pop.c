
#include <Module/nDigitAry.h>

DEFUALT_DIGIT
DigitAry_Pop(nDigitAry_ptr pSelf)
{
  if (pSelf->m_Length == 0)
    return -1;

  DEFUALT_DIGIT _Ret = DigitAry_get(pSelf, pSelf->m_Length);
  DigitAry_Remove(pSelf, pSelf->m_Length);
  return _Ret;
}
