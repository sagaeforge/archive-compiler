
#include <Chs.h>
#include <Private_String.h>

String
String_ToLower(String Self)
{
  wcs temp = __WcsCreate(Self->Length);

  int i;
  for (i = 0; i < Self->Length; i++)
    temp[i] = __ToLower(Self->Value[i]);
  temp[i] = '\0';
  return String(temp);
}
