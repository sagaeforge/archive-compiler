
#include "Chs.h"
#include "Private_String.h"
#include "ProgramManager.h"

static bool
_StringCompare(wcs Ary, String* FindValue, Index Start)
{
  int i;
  for (i = Start; i < Start + FindValue->Length; i++)
    if (Ary[i] != FindValue->Value[i - Start])
      return false;
  return true;
}

String*
String_ReplaceAll(String* Self, String* Ori, String* Value)
{
  Length calcvalue = String_Count(Self, Ori);
  if (calcvalue == 0)
    return String(Self);
  Length leng =
    Self->Length - (Ori->Length * calcvalue) + (Value->Length * calcvalue);
  wcs temp = __WcsCreate(leng);
  bool IsNull = String_IsNone(Value);
  int i, j, temp_Pos = 0;
  for (i = 0; i < Self->Length; i++)
    if (_StringCompare(Self->Value, Ori, i)) {
      if (!IsNull) {
        for (j = 0; j < Value->Length; j++)
          temp[i + j + temp_Pos] = Value->Value[j];
        i += Ori->Length;
      } else {
        temp_Pos--;
        continue;
      }
    } else
      temp[i + temp_Pos] = Self->Value[i];
  temp[i] = L'\0';
  return String(temp);
}
