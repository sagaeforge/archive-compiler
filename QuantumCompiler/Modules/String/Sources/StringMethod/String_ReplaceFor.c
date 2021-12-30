
#include "Chs.h"
#include "Private_String.h"
#include "ProgramManager.h"

static bool _StringCompare(wcs Ary, String *FindValue, Index Start) {
  int i;
  for (i = Start; i < Start + FindValue->Length; i++)
    if (Ary[i] != FindValue->Value[i])
      return false;
  return true;
}

String *String_ReplaceFor(String *Self, String *Ori, String *Value,
                          Length Length) {
  if (Length > String_Count(Self, Ori))
    return String(Self);

  int leng = Self->Length - (Ori->Length * Length) + (Value->Length * Length);
  wcs temp = __WcsCreate(leng);
  int i, j, total = 0;
  for (i = 0; i < leng; i++)
    if (total < Length && _StringCompare(Self->Value, Ori, i)) {
      for (j = 0; j < Value->Length; j++)
        temp[i + j] = Value->Value[j];
      i += Ori->Length;
      total++;
    } else
      temp[i] = Self->Value[i];
  temp[i] = L'\0';
  return String(temp);
}
