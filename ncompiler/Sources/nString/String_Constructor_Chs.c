
#include <stdlib.h>

#include <Exception.h>
#include <Module/nString.h>

nString_t*
String_Constructor_Chs(const Chs_t pValue)
{
  Length_t _Len = 0;
  Wcs_t _BUF = String_UTF8Decorder(pValue, &_Len);
  if (!_BUF) {
    Exception(ERROR, "nString을 생성할 수 없습니다.");
    return NULL;
  }

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
