
#include "Private_StringLib.h"

static String True;
bool
String_ValueOf_Bool(String Self)
{
  if (StringMethod.IsNone(True))
    True = String("true");

  return StringMethod.Compare(Self, True);
}
