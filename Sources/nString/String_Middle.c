
#include <Module/nString.h>

nString_t*
String_Middle(const nString_t* pSelf, const Index_t pStart, Index_t pEnd)
{
  return String_Extract(pSelf, pStart, pEnd);
}
