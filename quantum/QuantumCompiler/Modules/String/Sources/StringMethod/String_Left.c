
#include <Chs.h>
#include <Private_String.h>
#include <Private_StringLib.h>

String
String_Left(String pSelf, Length_t pLength)
{
  if (pLength >= pSelf->m_Length)
    return String(pSelf);

  return String_Extract(pSelf, 0, pLength);
}