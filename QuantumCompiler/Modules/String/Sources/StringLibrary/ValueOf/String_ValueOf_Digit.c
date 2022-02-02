
#include <Private_StringLib.h>

double
String_ValueOf_Digit(String Self)
{
  wcs EndPos = NULL;
  return wcstold(Self->Value, &EndPos);
}
