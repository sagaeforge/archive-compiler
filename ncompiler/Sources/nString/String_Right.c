
#include <Module/nString.h>

nString_t*
String_Right(const nString_t* pSelf, const Length_t pLength)
{
  return String_Extract(pSelf, pLength, pSelf->m_Length);
}
