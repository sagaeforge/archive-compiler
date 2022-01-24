
#include "Private_StringLib.h"

String*
String_Notation(int64_t Value, int notation)
{
  if (notation != 2 && notation != 8 && notation != 16)
    return String_ToString_Decimal_Unsigned(Value);

  int i = 0, len = 0;
  wchar_t temp[25];
  wchar_t revs[25];
  char ch;

  if (notation == 2)
    ch = 'b';
  else if (notation == 8)
    ch = 'o';
  else
    ch = 'x';

  revs[0] = '0';
  revs[1] = ch;

  while (Value >= notation) {
    temp[i] = Value % notation;
    Value /= notation;
    i++;
  }
  temp[i] = Value % notation;
  len = i + 1;
  for (i = 0; i < len; i++)
    if (notation != 16)
      temp[i] += '0';
    else
      temp[i] += temp[i] >= 10 ? 'A' - 10 : '0';

  // 반전시키는 코드가 들어가야함
  for (i = 0; i < len; i++)
    revs[2 + i] = temp[len - 1 - i];
  revs[2 + len] = '\0';

  return String(revs);
}