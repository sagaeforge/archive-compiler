
#include <Module/nString.h>
#include <Module/nStringAry.h>

nString_t*
String_ToString_StringAry(const nStringAry_t* pValue, const nString_t* pReplaceWord)
{
  nString_ptr _str = nString("");
  Index_t _i;
  for (_i = 0; _i < pValue->m_Length; _i++) {
    String_Append(_str, StringAry_get(pValue, _i));
    String_Append(_str, pReplaceWord);
  }
  return _str;
}
