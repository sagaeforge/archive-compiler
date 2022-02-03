
#include <Private_StringLib.h>

int64_t
String_ValueOf_Decimal(String pSelf)
{
  if (pSelf->Value[0] != '-')
    return String_ValueOf_Decimal_Unsigned(pSelf);

  uint64_t ret = 0;
  int i;
  for (i = 1; i < pSelf->Length - 1; i++) {
    ret += pSelf->Value[i] - L'0';
    ret *= 10;
  }
  ret += pSelf->Value[i] - L'0';
  return ret;
}