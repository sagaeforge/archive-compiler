
#include "Chs.h"
#include "Private_StringLib.h"

bool String_IsUpper(String *Self) {
  int i;
  for (i = 0; i < Self->Length; i++)
    if (!__IsUpper(Self->Value[i]))
      return false;
  return true;
}