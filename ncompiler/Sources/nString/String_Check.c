
#include <Module/nString.h>

bool
String_Check(const wchar_t pChar, const nString_t* pFindAry)
{
  Index_t _i;
  for (_i = 0; _i < pFindAry->m_Length; _i++)
    if (pChar == pFindAry->m_Value[_i])
      return true;
  return false;
}
