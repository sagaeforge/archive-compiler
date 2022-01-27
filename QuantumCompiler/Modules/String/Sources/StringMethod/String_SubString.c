
#include <Private_String.h>
#include <Private_StringLib.h>

String
String_SubString(String Self, String Value)
{
  int ind = String_IndexOf(Self, Value);
  if (ind == -1)
    return String(Self);

  return String_Extract(Self, 0, ind);
}
