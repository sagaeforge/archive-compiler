
#include <Private_StringLib.h>

String
String_Prettier(double pValue)
{
  wchar_t temp[300];
  swprintf(temp, 300, L"%g", pValue);
  return String(temp);
}