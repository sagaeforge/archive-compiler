
#include <Module/nString.h>

nString_t*
String_Trim(const nString_t* pSelf)
{
  Index_t _i, _Start = 0, _End = 0;
  for (_i = 0; _i < pSelf->m_Length; _i++)
    if (__IsSpace(pSelf->m_Value[_i]))
      _Start++;
    else
      break;
  if (_Start == pSelf->m_Length)
    return nString("");
  for (_i = pSelf->m_Length - 1; !LOOPEND(_i); _i--)
    if (__IsSpace(pSelf->m_Value[_i]))
      _End++;
    else
      break;
  return String_Extract(pSelf, _Start, _End);
}
