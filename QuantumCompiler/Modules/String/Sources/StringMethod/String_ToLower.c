
#include <Chs.h>
#include <Private_String.h>

String
String_ToLower(String pSelf)
{
  wcs temp = __WcsCreate(pSelf->Length);

  int i;
  for (i = 0; i < pSelf->Length; i++)
    temp[i] = __ToLower(pSelf->Value[i]);
  temp[i] = '\0';
  return String(temp);
}
