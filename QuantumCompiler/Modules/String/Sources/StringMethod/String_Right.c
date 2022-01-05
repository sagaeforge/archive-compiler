
#include "Chs.h"
#include "Private_String.h"
#include "ProgramManager.h"

String *String_Right(String *Self, Length Length) {
  if (Length >= Self->Length)
    return String(Self);

  wcs temp = __WcsCreate(Length);
  int i, total = Length;
  for (i = Self->Length; i >= Self->Length - Length; i--, total--)
    temp[total] = Self->Value[i];

  return String(temp);
}
