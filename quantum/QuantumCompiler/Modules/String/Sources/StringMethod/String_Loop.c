
#include <Chs.h>
#include <Private_String.h>

String
String_Loop(String pSelf, Length_t pLength)
{
  wcs temp = __WcsCreate(pSelf->m_Length * pLength);

  int i;
  for (i = 0; i < pLength; i++)
    __WcsWcsInsert(temp, pSelf->m_Value, pSelf->m_Length * i, pSelf->m_Length);

  return String(temp);
}