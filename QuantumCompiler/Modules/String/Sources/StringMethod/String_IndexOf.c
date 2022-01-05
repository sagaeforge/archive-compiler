
#include "Private_String.h"
#include "ProgramManager.h"

// TODO 최적화
static bool _StringCompare(wcs Ary, String *FindValue, Index Start) {
  int i;
  for (i = Start; i < Start + FindValue->Length; i++)
    if (Ary[i] != FindValue->Value[i - Start])
      return false;
  return true;
}

int String_IndexOf(String *Self, String *Value) {
  int i;
  for (i = 0; i < Self->Length - Value->Length + 1; i++)
    if (Self->Value[i] == Value->Value[0])
      if (_StringCompare(Self->Value, Value, i))
        return i;
  return -1;
}