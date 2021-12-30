
#include "Chs.h"
#include "Private_String.h"
#include "ProgramManager.h"

String *String_Replace(String *Self, String *Ori, String *Value) {
  int ind = String_IndexOf(Self, Ori);
  if (ind == -1)
    return String(Self);

  Length leng = Self->Length - Ori->Length + Value->Length;
  wcs temp = __WcsCreate(leng);
  int i, j;
  for (i = 0; i < leng; i++)
    if (i == ind) {
      for (j = 0; j < Value->Length; j++)
        temp[i + j] = Value->Value[j];
      i += Ori->Length;
    } else
      temp[i] = Self->Value[i];
  temp[i] = L'\0';
  return String(temp);
}
