
#include <Private_StringLib.h>

uint64_t
String_ValueOf_Decimal_Unsigned(String Self)
{
  uint64_t ret = 0;
  int i;
  for (i = 0; i < Self->Length - 1; i++) {
    ret += Self->Value[i] - L'0';
    ret *= 10;
  }
  ret += Self->Value[i] - L'0';
  return ret;
}