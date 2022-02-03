
#include <Private_String.h>
#include <Private_StringLib.h>

String
String_SubString(String pSelf, String pValue)
{
  int ind = String_IndexOf(pSelf, pValue);
  if (ind == -1)
    return String(pSelf);

  return String_Extract(pSelf, 0, ind);
}
