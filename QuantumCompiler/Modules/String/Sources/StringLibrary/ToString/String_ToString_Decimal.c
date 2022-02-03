
#include <Private_StringLib.h>

String
String_ToString_Decimal(int64_t pValue)
{
  if (pValue > 0)
    String_ToString_Decimal_Unsigned(pValue);

  wchar_t temp[30];
  swprintf(temp, 30, L"%ld", pValue);
  return String(temp);
}