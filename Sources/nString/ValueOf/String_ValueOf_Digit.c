
#include <Module/nString.h>
#include <Module/nStringAry.h>

double
String_ValueOf_Digit(const nString_t* pSelf)
{
  Wcs_t EndPos = NULL;
  return wcstold(pSelf->m_Value, &EndPos);
}
