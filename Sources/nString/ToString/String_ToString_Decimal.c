
#include <Module/nString.h>

nString_t*
String_ToString_Decimal(const int64_t pValue)
{
  if (pValue > 0)
    String_ToString_Decimal_Unsigned(pValue);

  wchar_t temp[30];
  swprintf(temp, 30, L"%ld", pValue);
  return nString(temp);
}
