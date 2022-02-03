
#include <Private_StringLib.h>

uint64_t
String_ValueOf_Decimal_Unsigned(String pSelf)
{
  uint64_t ret = 0;
  int i;
  for (i = 0; i < pSelf->Length - 1; i++) {
    ret += pSelf->Value[i] - L'0';
    ret *= 10;
  }
  ret += pSelf->Value[i] - L'0';
  return ret;
}