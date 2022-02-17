
#include <stdlib.h>

#include <Exception.h>
#include <Module/nString.h>

nString_t*
String_ToUpper(const nString_t* pSelf)
{
  __WCSBUFMAKE(pSelf->m_Length);
  if (!_BUF) {
    Exception(ERROR, "nString을 생성할 수 없습니다.");
    return false;
  }

  Index_t _i;
  for (_i = 0; _i < pSelf->m_Length; _i++)
    _BUF[_i] = __ToLower(pSelf->m_Value[_i]);

  nString_ptr _ptr = nString(_BUF);
  __FLUSHWCSBUF();
  return _ptr;
}
