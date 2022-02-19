
#include <stdlib.h>

#include <Module/nString.h>

nString_t*
String_ReplaceAt(const nString_t* pSelf, const nString_t* pKeyWord, const nString_t* pValue, const Index_t pIndex)
{
  Index_t _Find = String_IndexAt(pSelf, pKeyWord, pIndex);
  if (_Find == -1)
    return nString(pSelf);

  Length_t _Length = pSelf->m_Length - pKeyWord->m_Length + pValue->m_Length;
  __WCSBUFMAKE(_Length);
  Index_t _i, _gap = 0;
  for (_i = 0; _i < _Length; _i++) {
    if (_i < _Find)
      _BUF[_i] = pSelf->m_Value[_i];
    else {
      if (_i > _Find + pValue->m_Length)
        _BUF[_i + _gap] = pSelf->m_Value[_i];
      else {
        Index_t _j;
        for (_j = 0; _j < pValue->m_Length; _j++)
          _BUF[_i + _j] = pValue->m_Value[_j];
        _i += pValue->m_Length - 1;
        _gap = pValue->m_Length - pKeyWord->m_Length;
      }
    }
  }

  nString_ptr _ptr = nString(_BUF);
  __FLUSHWCSBUF();
  return _ptr;
}
