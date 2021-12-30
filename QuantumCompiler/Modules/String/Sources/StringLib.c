
#include "StringLib.h"
#include "Chs.h"
#include <stdio.h>
#include <wchar.h>

// TODO 파일이 너무 커질경우 분할

String *ToString_Bool(bool Value) {
  return Value ? String("true") : String("false");
}
String *ToString_Decimal(_int64 Value) {
  if (Value > 0)
    ToString_Decimal_Unsigned(Value);

  wchar_t temp[30];
  swprintf(temp, 30, L"%lld", Value);
  return String(temp);
}
String *ToString_Decimal_Unsigned(_uint64 Value) {
  wchar_t temp[30];
  swprintf(temp, 30, L"%llu", Value);
  return String(temp);
}
String *ToString_Digit(double Value) {
  wchar_t temp[300];
  swprintf(temp, 300, L"%lf", Value);
  return String(temp);
}

static String *True;
bool ValueOf_Bool(String *Self) {
  if (StringMethod.IsNone(True))
    True = String("true");

  return StringMethod.Compare(Self, True);
}

_int64 ValueOf_Decimal(String *Self) {
  // TODO 최적화, 오류 검사
  if (Self->Value[0] != '-')
    ValueOf_Decimal_Unsigned(Self);

  _int64 ret = 0;
  int i;
  for (i = 1; i < Self->Length - 1; i++) {
    ret += Self->Value[i] - L'0';
    ret *= 10;
  }
  ret += Self->Value[i] - L'0';
  return ret;
}
_uint64 ValueOf_Decimal_Unsigned(String *Self) {
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
double ValueOf_Digit(String *Self) {
  // TODO 최적화, 오류 검사
  wcs EndPos = NULL;
  return wcstold(Self->Value, &EndPos);
}