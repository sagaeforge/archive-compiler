
#include "Private_StringLib.h"

String*
String_Prettier(double Value)
{
  wchar_t temp[300];
  swprintf(temp, 300, L"%g", Value);
  return String(temp);
}