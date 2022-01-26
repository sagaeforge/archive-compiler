
#include "Private_String.h"

// TODO 최적화
static bool
_StringCompare(wcs Ary, String FindValue, Index_t Start)
{
  int i;
  for (i = Start; i < Start + FindValue->Length; i++)
    if (Ary[i] != FindValue->Value[i - Start])
      return false;
  return true;
}

Index_t
String_IndexFor(String Self, String Value, Index_t Index)
{
  Length_t len = String_Count(Self, Value);
  if (len == 0 || len <= Index)
    return -1;

  int i, cnt = 0;
  for (i = 0; i < Self->Length; i++) {
    if (Self->Value[i] == Value->Value[0])
      if (_StringCompare(Self->Value, Value, i)) {
        if (cnt != Index)
          cnt++;
        else
          return i;
      }
  }
  return -1;
}