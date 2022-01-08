
#include "Private_StringLib.h"

String *String_ToString_Decimal_Unsigned(_uint64 Value) {
  wchar_t temp[30];
  swprintf(temp, 30, L"%llu", Value);
  return String(temp);
}