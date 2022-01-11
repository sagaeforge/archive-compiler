
#include "Chs.h"
#include "Private_StringLib.h"

String*
String_Extract(String* Self, Index Start, Index End)
{
  if (Self->Length < End)
    return String(Self);

  Length leng = End - Start + 1;
  wchar_t* temp = __WcsCreate(leng);
  int i;
  for (i = Start; i < End; i++)
    temp[i - Start] = Self->Value[i];
  temp[i - Start] = '\0';
  return String(temp);
}
