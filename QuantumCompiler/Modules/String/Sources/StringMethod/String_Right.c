
#include <Chs.h>
#include <Private_String.h>
#include <Private_StringLib.h>

String
String_Right(String pSelf, Length_t pLength)
{
  if (pLength >= pSelf->Length)
    return String(pSelf);

  return String_Extract(pSelf, pSelf->Length - pLength, pSelf->Length);
}
