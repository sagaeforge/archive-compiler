
#include "Chs.h"
#include "Private_String.h"
#include "ProgramManager.h"

String*
String_Middle(String* Self, Index Start, Index Count)
{
  // TODO 길이 검사
  Length len = Count - Start + 1;
  if (Start + len >= Self->Length)
    return String(Self);

  wcs temp = __WcsCreate(len);

  int i, total = 0;
  for (i = Start; i < Start + Count; i++, total++)
    temp[total] = Self->Value[i];
  temp[total] = '\0';
  return String(temp);
}