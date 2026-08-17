
#include <Private_StringLib.h>

String
String_ToString_Digit(double pValue)
{
  wchar_t temp[300];
  swprintf(temp, 300, L"%lf", pValue);
  return String(temp);
}