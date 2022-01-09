
#include "Chs.h"
#include "Private_String.h"
#include "ProgramManager.h"

String*
String_SubString(String* Self, String* Value)
{
  int ind = String_IndexOf(Self, Value);
  if (ind == -1)
    return String(Self);

  wcs temp = __WcsCreate(ind);

  int i;
  for (i = 0; i < ind; i++)
    temp[i] = Self->Value[i];
  temp[i] = L'\0';

  return String(temp);
}
