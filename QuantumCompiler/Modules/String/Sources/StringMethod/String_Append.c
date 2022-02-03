
#include <Chs.h>
#include <GarbageCollection.h>
#include <Private_String.h>

void
String_Append(String pSelf, String pValue)
{
  if (pValue->IsNone)
    return;

  wcs temp = __WcsCreate(pSelf->Length + pValue->Length);
  __WcsWcsInsert(temp, pSelf->Value, 0, pSelf->Length);
  __WcsWcsInsert(temp, pValue->Value, pSelf->Length, pValue->Length);
  String_Set(pSelf, String(temp));
}
