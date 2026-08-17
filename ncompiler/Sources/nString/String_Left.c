
#include <Module/nString.h>

nString_t*
String_Left(const nString_t* pSelf, const Length_t pLength)
{
  return String_Extract(pSelf, 0, pLength);
}
