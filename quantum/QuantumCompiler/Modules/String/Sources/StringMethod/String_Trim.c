
#include <Chs.h>
#include <Private_String.h>
#include <Private_StringLib.h>

static bool
_IsSpaceChs(wchar_t pCh)
{
  return (pCh >= 9 && pCh <= 13) || pCh == 32;
}

String
String_Trim(String pSelf)
{
  int i, space_Front = 0, space_Rear = 0;
  for (i = 0; i < pSelf->m_Length; i++)
    if (!_IsSpaceChs(pSelf->m_Value[i]))
      break;
    else
      space_Front++;
  if (space_Front == pSelf->m_Length)
    return String("");

  for (i = pSelf->m_Length - 1; i >= 0; i--)
    if (!_IsSpaceChs(pSelf->m_Value[i]))
      break;
    else
      space_Rear++;

  return String_Extract(pSelf, space_Front, pSelf->m_Length - space_Rear);
}
