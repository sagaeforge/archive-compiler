
#include <Chs.h>
#include <GarbageCollection.h>
#include <Private_String.h>

void
String_Set(String pSelf, String pValue)
{
  if (pSelf->IsNone)
    pSelf->Value = pValue->Value;
  else {
    MemoryRemove(pSelf->Value);
    wcs temp = __WcsCreate(pValue->Length);
    __StrSet(temp, pValue->Value, 4, pValue->Length);
    pSelf->Value = temp;
  }

  pSelf->IsNone = pValue->IsNone;
  pSelf->Length = pValue->Length;
}
