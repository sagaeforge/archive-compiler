
#include <stdlib.h>

#include <Module/nString.h>

nString_t*
String_SubString(const nString_t* pSelf, const nString_t* pValue)
{
  Index_t _Find = String_IndexOf(pSelf, pValue);
  if (_Find == -1)
    return nString(pSelf);

  return String_Left(pSelf, _Find);
}
