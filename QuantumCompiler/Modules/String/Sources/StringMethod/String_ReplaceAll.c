
#include "Private_String.h"
#include "ProgramManager.h"

static bool _StringCompare(wcs Ary, String *FindValue, Index Start) {
  int i;
  for (i = Start; i < Start + FindValue->Length; i++)
    if (Ary[i] != FindValue->Value[i])
      return false;
  return true;
}

String *String_ReplaceAll(String *Self, String *Ori, String *Value) {
  Length calcvalue = String_Count(Self, Ori);
  if (calcvalue == 0)
    return String(Self);
  Length leng =
      Self->Length - (Ori->Length * calcvalue) + (Value->Length * calcvalue);
  wcs temp = __WcsCreate(leng);
  int i, j;
  for (i = 0; i < leng; i++)
    if (_StringCompare(Self->Value, Ori, i)) {
      for (j = 0; j < Value->Length; j++)
        temp[i + j] = Value->Value[j];
      i += Ori->Length;
    } else
      temp[i] = Self->Value[i];
  temp[i] = L'\0';
  return String(temp);
}
