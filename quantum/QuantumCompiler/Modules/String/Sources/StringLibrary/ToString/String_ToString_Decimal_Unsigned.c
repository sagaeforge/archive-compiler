
#include <Private_StringLib.h>

String
String_ToString_Decimal_Unsigned(uint64_t pValue)
{
  wchar_t temp[30];
  swprintf(temp, 30, L"%lu", pValue);
  return String(temp);
}