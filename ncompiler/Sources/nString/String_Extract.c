
#include <stdlib.h>

#include <Exception.h>
#include <Module/nString.h>

nString_t*
String_Extract(const nString_t* pSelf, const Index_t pStart, const Index_t pEnd)
{
  if (pStart == 0 && pEnd == pSelf->m_Length - 1)
    return nString(pSelf);

  Index_t _End = pEnd >= pSelf->m_Length ? pSelf->m_Length - 1 : pEnd;

  __WCSBUFMAKE(pStart + pEnd - pStart);
  if (!_BUF) {
    Exception(ERROR, "nString을 생성할 수 없습니다.");
    return NULL;
  }
  Index_t _i, Pointer = 0;
  for (_i = pStart; _i < _End; _i++)
    _BUF[Pointer++] = pSelf->m_Value[_i];

  nString_ptr _ptr = nString(_BUF);
  __FLUSHWCSBUF();
  return _ptr;
}
