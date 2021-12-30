
#include "Chs.h"
#include "Private_String.h"
#include "ProgramManager.h"

String *String_Middle(String *Self, Index Start, Index End) {
  // TODO 길이 검사
  Length len = End - Start + 1;
  wcs temp = __WcsCreate(len);

  int i, total = 0;
  for (i = Start; i <= End; i++, total++)
    temp[total] = Self->Value[i];
  temp[total] = '\0';
  return String(temp);
}