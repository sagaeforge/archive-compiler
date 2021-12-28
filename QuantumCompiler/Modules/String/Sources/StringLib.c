
#include "StringLib.h"
#include "Chs.h"
#include <stdio.h>
#include <wchar.h>

String *ToString_Bool(bool Value) {
  return Value ? String("true") : String("false");
}
String *ToString_Decimal(_int64 Value) {
  wchar_t temp[30];
  swprintf(temp, 30, L"%lld", Value);
  return String(temp);
}
String *ToString_Digit(double Value) {
  wchar_t temp[300];
  swprintf(temp, 300, L"%lf", Value);
  return String(temp);
}