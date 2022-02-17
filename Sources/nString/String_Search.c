
#include <Module/nString.h>
#include <Module/nStringAry.h>

bool
String_Search(const nString_t* pSelf, const nStringAry_t* pFindAry)
{
  Index_t _i;
  for (_i = 0; _i < pFindAry->m_Length; _i++)
    if (!String_Compare(pSelf, StringAry_get(pFindAry, _i)))
      return false;
  return true;
}
