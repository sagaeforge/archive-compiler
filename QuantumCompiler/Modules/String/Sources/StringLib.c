
#include "StringLib.h"
#include "Chs.h"
#include <stdio.h>
#include <wchar.h>

String *ToString_Bool(bool Value) {
  return Value ? String("true") : String("false");
}
String *ToString_Decimal(_int64 Value) {
  if (Value >= 0)
    return ToString_Decimal_unsigned(Value);

  Value *= -1;
  _int64 Backup = Value;

  // 자리 구하기
  int leng = 1;
  if (Value != 0) {
    while (Value != 0) {
      Value /= 10;
      leng++;
    }
  } else
    leng++;

  Value = Backup;
  wcs temp = __WcsCreate(leng);
  temp[0] = '-';
  temp[leng] = '\0';

  int i;
  for (i = 1; i < leng; i++) {
    temp[i] = Value % 10 + '0';
    Value /= 10;
  }
  return String(temp);
}
String *ToString_Decimal_unsigned(_uint64 Value) {
  _int64 Backup = Value;

  // 자리 구하기
  int leng = 0;
  if (Value != 0) {
    while (Value != 0) {
      Value /= 10;
      leng++;
    }
  } else
    leng++;

  Value = Backup;
  wcs temp = __WcsCreate(leng);
  temp[leng] = '\0';

  int i;
  for (i = 0; i < leng; i++) {
    temp[i] = Value % 10 + '0';
    Value /= 10;
  }
  return String(temp);
}
String *ToString_Digit(double Value) {
  wchar_t temp[300];
  swprintf(temp, 300, L"%lf", Value);
  return String(temp);
}