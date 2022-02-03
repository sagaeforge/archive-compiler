
#include <Chs.h>
#include <Private_String.h>

String
String_Loop(String pSelf, Length_t pLength)
{
  wcs temp = __WcsCreate(pSelf->Length * pLength);

  int i, j;
  for (i = 0; i < pLength; i++)
    __WcsWcsInsert(temp, pSelf->Value, pSelf->Length * i, pSelf->Length);

  return String(temp);
}