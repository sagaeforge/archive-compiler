
#include <Private_String.h>

Index_t
String_IndexOf(String Self, String Value)
{
  int i;
  for (i = 0; i < Self->Length - Value->Length + 1; i++)
    if (Self->Value[i] == Value->Value[0])
      if (_StringCompare(Self->Value, Value, i))
        return i;
  return -1;
}