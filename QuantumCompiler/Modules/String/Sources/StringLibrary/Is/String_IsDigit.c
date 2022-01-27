
#include <Chs.h>
#include <Private_StringLib.h>

bool
String_IsDigit(String Self)
{
  int i, dotCount = 0, ECount = 0;
  for (i = 0; i < Self->Length; i++) {
    if (Self->Value[i] == '.') {
      if (dotCount != 0)
        return false;
      dotCount++;
    } else if (Self->Value[i] == 'E') {
      if (ECount != 0)
        return false;
      ECount++;
    } else if (Self->Value[i] == '+') {
      if (i != 0 || (i != 0 && Self->Value[i - 1] != 'E'))
        return false;
    } else if (Self->Value[i] == '-') {
      if (i != 0 || (i != 0 && Self->Value[i - 1] != 'E'))
        return false;
    } else if (!__IsDecimal(Self->Value[i]))
      return false;
  }
  return true;
}