
#include "Chs.h"
#include "Private_StringLib.h"

String
String_Reverse(String Self)
{
  wchar_t* temp = __WcsCreate(Self->Length);
  int i;
  for (i = Self->Length - 1; i >= 0; i--)
    temp[Self->Length - (i + 1)] = Self->Value[i];
  temp[Self->Length] = '\0';
  return String(temp);
}
