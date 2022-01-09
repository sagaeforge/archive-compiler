
#include "Chs.h"
#include "Private_StringLib.h"

bool
String_IsSpace(String* Self)
{
  int i;
  for (i = 0; i < Self->Length; i++)
    if (!__IsSpace(Self->Value[i]))
      return false;
  return true;
}