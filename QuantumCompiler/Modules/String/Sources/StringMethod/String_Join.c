
#include <Chs.h>
#include <Private_String.h>

String
String_Join(String pSelf, String pValue)
{
  wcs temp = __WcsCreate(pSelf->Length + pValue->Length);

  __WcsWcsInsert(temp, pSelf->Value, 0, pSelf->Length);
  __WcsWcsInsert(temp, pValue->Value, pSelf->Length - 1, pValue->Length);

  return String(temp);
}
