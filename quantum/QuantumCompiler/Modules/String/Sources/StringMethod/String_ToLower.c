
#include <Chs.h>
#include <Private_String.h>

String
String_ToLower(String pSelf)
{
  wcs temp = __WcsCreate(pSelf->m_Length);

  int i;
  for (i = 0; i < pSelf->m_Length; i++)
    temp[i] = __ToLower(pSelf->m_Value[i]);
  temp[i] = '\0';
  return String(temp);
}
