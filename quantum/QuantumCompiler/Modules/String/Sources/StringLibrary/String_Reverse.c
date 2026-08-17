
#include <Chs.h>
#include <Private_StringLib.h>

String
String_Reverse(String pSelf)
{
  wchar_t* temp = __WcsCreate(pSelf->m_Length);
  int i;
  for (i = pSelf->m_Length - 1; i >= 0; i--)
    temp[pSelf->m_Length - (i + 1)] = pSelf->m_Value[i];
  temp[pSelf->m_Length] = '\0';
  return String(temp);
}
