
#include <stdlib.h>

#include <Exception.h>
#include <Module/nString.h>

bool
String_set(nString_t* pSelf, const nString_t* pValue)
{
  Index_t _i;
  if (pSelf->m_Length == pValue->m_Length) {
    for (_i = 0; _i < pSelf->m_Length; _i++)
      pSelf->m_Value[_i] = pValue->m_Value[_i];
  } else {
    free(pSelf->m_Value);
    __WCSBUFMAKE(pValue->m_Length);
    if (!_BUF) {
      Exception(ERROR, "nString을 생성할 수 없습니다.");
      return false;
    }
    for (_i = 0; _i < pValue->m_Length; _i++)
      _BUF[_i] = pValue->m_Value[_i];
    pSelf->m_Value = _BUF;
  }
  pSelf->m_Length = pValue->m_Length;
  return true;
}
