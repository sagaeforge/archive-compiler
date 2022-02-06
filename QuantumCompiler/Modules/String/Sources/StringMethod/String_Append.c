
#include <Chs.h>
#include <GarbageCollection.h>
#include <Private_String.h>

void
String_Append(String pSelf, String pValue)
{
  if (pValue->m_IsNone)
    return;

  wcs temp = __WcsCreate(pSelf->m_Length + pValue->m_Length);
  __WcsWcsInsert(temp, pSelf->m_Value, 0, pSelf->m_Length);
  __WcsWcsInsert(temp, pValue->m_Value, pSelf->m_Length, pValue->m_Length);
  String_Set(pSelf, String(temp));
}
