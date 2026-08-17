
#include <stdlib.h>

#include <Exception.h>
#include <Module/nString.h>

bool
String_Append(nString_t* pSelf, const nString_t* pValue)
{
  free(pSelf->m_Value);
  __WCSBUFMAKE(pSelf->m_Length + pValue->m_Length);
  if (!_BUF) {
    Exception(ERROR, "nString을 생성할 수 없습니다.");
    return false;
  }

  Index_t _i;
  for (_i = 0; _i < pSelf->m_Length; _i++)
    _BUF[_i] = pSelf->m_Value[_i];
  for (; _i < pValue->m_Length; _i++)
    _BUF[_i] = pValue->m_Value[_i - pValue->m_Length];

  pSelf->m_Length += pValue->m_Length;
  pSelf->m_Value = _BUF;
  return true;
}
