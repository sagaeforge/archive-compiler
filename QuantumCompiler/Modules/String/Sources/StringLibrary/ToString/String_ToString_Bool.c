
#include <Private_StringLib.h>

String
String_ToString_Bool(bool pValue)
{
  return pValue ? String("true") : String("false");
}