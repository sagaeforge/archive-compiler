
#include <stdlib.h>

#include <Exception.h>
#include <Module/nString.h>

nString_t*
String_Constructor_Wcs(const Wcs_t pValue)
{
  Length_t _Len = __STRLEN(pValue);
  __WCSBUFMAKE(_Len);
  if (!_BUF) {
    Exception(ERROR, "nString을 생성할 수 없습니다.");
    return NULL;
  }
  Index_t _i;
  for (_i = 0; _i < _Len; _i++)
    _BUF[_i] = pValue[_i];

  nString_ptr _Str = (nString_ptr)malloc(sizeof(nString_t));
  if (!_Str) {
    Exception(ERROR, "nString을 생성할 수 없습니다.");
    __FLUSHWCSBUF();
    return NULL;
  }

  _Str->m_Length = _Len;
  _Str->m_Value = _BUF;

  return _Str;
}
