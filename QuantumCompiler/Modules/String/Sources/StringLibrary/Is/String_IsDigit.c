
#include <Chs.h>
#include <Private_StringLib.h>

bool
String_IsDigit(String pSelf)
{
  int i, dotCount = 0, ECount = 0;
  for (i = 0; i < pSelf->Length; i++) {
    if (pSelf->Value[i] == '.') {
      if (dotCount != 0)
        return false;
      dotCount++;
    } else if (pSelf->Value[i] == 'E') {
      if (ECount != 0)
        return false;
      ECount++;
    } else if (pSelf->Value[i] == '+') {
      if (i != 0 || (i != 0 && pSelf->Value[i - 1] != 'E'))
        return false;
    } else if (pSelf->Value[i] == '-') {
      if (i != 0 || (i != 0 && pSelf->Value[i - 1] != 'E'))
        return false;
    } else if (!__IsDecimal(pSelf->Value[i]))
      return false;
  }
  return true;
}