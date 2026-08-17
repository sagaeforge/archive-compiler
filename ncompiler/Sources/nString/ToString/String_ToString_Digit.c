
#include <Module/nString.h>

nString_t*
String_ToString_Digit(const double pValue, const int numDigit)
{
  wchar_t temp[300];
  swprintf(temp, 300, L"%lg", pValue);
  return nString(temp);
}
