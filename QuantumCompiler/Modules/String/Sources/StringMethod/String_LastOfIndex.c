
#include "Private_String.h"

// TODO 최적화
static bool
_StringCompare(wcs Ary, String* FindValue, Index_t Start)
{
  int i;
  for (i = Start; i < Start + FindValue->Length; i++)
    if (Ary[i] != FindValue->Value[i - Start])
      return false;
  return true;
}

Index_t
String_LastOfIndex(String* Self, String* Value)
{
  int i;
  for (i = Self->Length - Value->Length; i >= 0; i--)
    if (Self->Value[i] == Value->Value[0])
      if (_StringCompare(Self->Value, Value, i))
        return i;
  return -1;
}