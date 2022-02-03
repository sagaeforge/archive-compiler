
#include <Chs.h>
#include <Private_String.h>
#include <Private_StringLib.h>

String
String_Middle(String pSelf, Index_t pStart, Index_t pEnd)
{
  Length_t len = pEnd - pStart + 1;
  if (pStart + len >= pSelf->Length)
    return String(pSelf);

  return String_Extract(pSelf, pStart, pEnd);
}