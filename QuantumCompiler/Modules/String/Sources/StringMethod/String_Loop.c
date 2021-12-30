
#include "Chs.h"
#include "Private_String.h"
#include "ProgramManager.h"

String *String_Loop(String *Self, Length Length) {
  wcs temp = __WcsCreate(Self->Length * Length);

  int i, j;
  for (i = 0; i < Length; i++)
    for (j = 0; j < Self->Length; j++)
      temp[i + j] = Self->Value[j];
  temp[i + j] = '\0';

  return String(temp);
}