
#include <Private_String.h>

Index_t
String_IndexAt(String Self, String Value, Index_t Start)
{
  if (Self->Length <= Start)
    return -1;

  int i;
  for (i = Start; i < Self->Length - Value->Length + 1; i++)
    if (Self->Value[i] == Value->Value[0])
      if (_StringCompare(Self->Value, Value, i))
        return i;
  return -1;
}