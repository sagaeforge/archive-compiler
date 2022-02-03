
#include <Private_StringLib.h>

double
String_ValueOf_Digit(String pSelf)
{
  wcs EndPos = NULL;
  return wcstold(pSelf->Value, &EndPos);
}
