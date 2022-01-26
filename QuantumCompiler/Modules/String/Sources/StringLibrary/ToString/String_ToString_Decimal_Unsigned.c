
#include "Private_StringLib.h"

String
String_ToString_Decimal_Unsigned(uint64_t Value)
{
  wchar_t temp[30];
  swprintf(temp, 30, L"%lu", Value);
  return String(temp);
}