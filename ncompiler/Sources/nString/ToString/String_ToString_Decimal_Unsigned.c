
#include <Module/nString.h>

nString_t*
String_ToString_Decimal_Unsigned(const uint64_t pValue)
{
  wchar_t temp[30];
  swprintf(temp, 30, L"%lu", pValue);
  return nString(temp);
}
