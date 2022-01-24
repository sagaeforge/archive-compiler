
#include "Private_StringLib.h"

String*
String_ToString_Digit(double Value)
{
  wchar_t temp[300];
  swprintf(temp, 300, L"%lf", Value);
  return String(temp);
}