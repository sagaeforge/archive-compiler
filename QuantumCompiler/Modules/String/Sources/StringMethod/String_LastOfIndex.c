
#include <Private_String.h>

Index_t
String_LastOfIndex(String Self, String Value)
{
  int i;
  for (i = Self->Length - Value->Length; i >= 0; i--)
    if (Self->Value[i] == Value->Value[0])
      if (_StringCompare(Self->Value, Value, i))
        return i;
  return -1;
}