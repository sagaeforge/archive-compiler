
#include <stdlib.h>

#include <Exception.h>
#include <Module/nString.h>

nString_t*
String_Reverse(const nString_t* pSelf)
{
  __WCSBUFMAKE(pSelf->m_Length);
  Index_t _i;
  for (_i = pSelf->m_Length - 1; !LOOPEND(_i); _i--)
    _BUF[_i - pSelf->m_Length - 1] = pSelf->m_Value[_i];

  nString_ptr _ptr = nString(pSelf);
  __FLUSHWCSBUF();
  return _ptr;
}
