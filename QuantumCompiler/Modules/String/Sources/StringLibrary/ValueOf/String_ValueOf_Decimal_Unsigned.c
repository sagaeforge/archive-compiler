
#include "Private_StringLib.h"

_uint64
String_ValueOf_Decimal_Unsigned(String* Self)
{
  // TODO 최적화, 오류 검사
  _int64 ret = 0;
  int i;
  for (i = 0; i < Self->Length - 1; i++) {
    ret += Self->Value[i] - L'0';
    ret *= 10;
  }
  ret += Self->Value[i] - L'0';
  return ret;
}