
#include "Chs.h"
#include "Private_String.h"
#include "ProgramManager.h"

// TODO 오류 검사
String*
String_Left(String* Self, Length Length)
{
  if (Length >= Self->Length)
    return String(Self);

  wcs temp = __WcsCreate(Length);

  int i;
  for (i = 0; i < Length; i++)
    temp[i] = Self->Value[i];
  temp[i] = '\0';
  return String(temp);
}