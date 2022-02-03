
#include <Private_StringLib.h>

static String True;
bool
String_ValueOf_Bool(String pSelf)
{
  if (StringMethod.IsNone(True))
    True = String("true");

  return StringMethod.Compare(pSelf, True);
}
