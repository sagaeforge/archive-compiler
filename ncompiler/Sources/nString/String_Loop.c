
#include <stdlib.h>

#include <Exception.h>
#include <Module/nString.h>

nString_t*
String_Loop(const nString_t* pSelf, const Length_t pLength)
{
  __WCSBUFMAKE(pSelf->m_Length * pLength);
  if (!_BUF) {
    Exception(ERROR, "nString을 생성할 수 없습니다.");
    return false;
  }

  Index_t _i, _j;
  for (_i = 0; _i < pLength; _i++)
    for (_j = 0; _j < pSelf->m_Length; _j++)
      _BUF[_i + _j] = pSelf->m_Value[_j];

  nString_ptr _ptr = nString(_BUF);
  __FLUSHWCSBUF();
  return _ptr;
}
