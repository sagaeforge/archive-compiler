
#include <Chs.h>
#include <GarbageCollection.h>
#include <Private_String.h>

void
String_Set(String pSelf, String pValue)
{
  if (pSelf->m_IsNone)
    pSelf->m_Value = pValue->m_Value;
  else {
    MemoryRemove(pSelf->m_Value);
    wcs temp = __WcsCreate(pValue->m_Length);
    __StrSet(temp, pValue->m_Value, 4, pValue->m_Length);
    pSelf->m_Value = temp;
  }

  pSelf->m_IsNone = pValue->m_IsNone;
  pSelf->m_Length = pValue->m_Length;
}
