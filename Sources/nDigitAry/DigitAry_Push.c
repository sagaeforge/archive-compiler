
#include <Module/nDigitAry.h>

void
DigitAry_Push(nDigitAry_ptr pSelf, DEFUALT_DIGIT pValue)
{
  DigitAry_Insert(pSelf, pSelf->m_Length, pValue);
}
