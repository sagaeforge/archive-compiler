
#include <Module/nString.h>

nString_t*
String_Constructor_Str(const nString_t pValue)
{
  return String_Constructor_Wcs(pValue.m_Value);
}
