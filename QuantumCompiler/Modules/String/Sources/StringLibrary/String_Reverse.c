
#include <Chs.h>
#include <Private_StringLib.h>

String
String_Reverse(String pSelf)
{
  wchar_t* temp = __WcsCreate(pSelf->Length);
  int i;
  for (i = pSelf->Length - 1; i >= 0; i--)
    temp[pSelf->Length - (i + 1)] = pSelf->Value[i];
  temp[pSelf->Length] = '\0';
  return String(temp);
}
