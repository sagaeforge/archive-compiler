
#include <Private_StringLib.h>

String
String_ToString_Decimal(int64_t Value)
{
  if (Value > 0)
    String_ToString_Decimal_Unsigned(Value);

  wchar_t temp[30];
  swprintf(temp, 30, L"%ld", Value);
  return String(temp);
}