
#include <Module/nString.h>

bool
String_ValueOf_Bool(const nString_t* pSelf)
{
  return String_Compare(pSelf, nString("true"));
}
