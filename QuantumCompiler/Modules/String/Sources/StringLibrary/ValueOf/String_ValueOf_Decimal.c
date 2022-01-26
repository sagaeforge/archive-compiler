
#include "Private_StringLib.h"

int64_t
String_ValueOf_Decimal(String Self)
{
  // TODO 최적화, 오류 검사
  if (Self->Value[0] != '-')
    String_ValueOf_Decimal_Unsigned(Self);

  uint64_t ret = 0;
  int i;
  for (i = 1; i < Self->Length - 1; i++) {
    ret += Self->Value[i] - L'0';
    ret *= 10;
  }
  ret += Self->Value[i] - L'0';
  return ret;
}