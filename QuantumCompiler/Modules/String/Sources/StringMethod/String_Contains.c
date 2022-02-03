
#include <Private_String.h>

bool
String_Contains(String pSelf, String pValue)
{
  return String_Count(pSelf, pValue) > 0;
}
