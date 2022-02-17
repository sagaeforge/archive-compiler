
#include <Module/nString.h>

nString_t*
String_ToString_Bool(const bool pValue)
{
  return pValue ? nString("true") : nString("false");
}
