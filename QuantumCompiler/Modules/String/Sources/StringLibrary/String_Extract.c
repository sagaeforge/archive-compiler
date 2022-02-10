
#include <Chs.h>
#include <Private_StringLib.h>

String
String_Extract(String pSelf, Index_t pStart, Index_t pEnd)
{
  if (pSelf->m_Length < pEnd - pStart)
    return String(pSelf);

  Length_t leng = pEnd - pStart + 1;
  wchar_t* temp = __WcsCreate(leng);
  int i;
  for (i = pStart; i < pEnd; i++)
    temp[i - pStart] = pSelf->m_Value[i];
  temp[i - pStart] = '\0';
  return String(temp);
}
