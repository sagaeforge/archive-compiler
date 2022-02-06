
#include <Chs.h>
#include <Private_String.h>

String
String_Join(String pSelf, String pValue)
{
  wcs temp = __WcsCreate(pSelf->m_Length + pValue->m_Length);

  __WcsWcsInsert(temp, pSelf->m_Value, 0, pSelf->m_Length);
  __WcsWcsInsert(temp, pValue->m_Value, pSelf->m_Length - 1, pValue->m_Length);

  return String(temp);
}
