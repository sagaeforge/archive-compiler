
#include "Chs.h"
#include "Private_String.h"
#include "ProgramManager.h"

// TODO 오류 검사
String *String_Join(String *Self, String *Value) {
  wcs temp = __WcsCreate(Self->Length + Value->Length);

  int i, total = 0;
  for (i = 0; i < Self->Length; i++, total++)
    temp[total] = Self->Value[i];
  for (i = 0; i < Value->Length; i++, total++)
    temp[total] = Value->Value[i];

  temp[total] = '\0';
  return String(temp);
}
