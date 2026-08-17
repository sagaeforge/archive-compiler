
#include <Private_StringLib.h>

String
String_Notation(int64_t pValue, int pNotation)
{
  if (pNotation != 2 && pNotation != 8 && pNotation != 16)
    return String_ToString_Decimal_Unsigned(pValue);

  int i = 0, len = 0;
  wchar_t temp[25];
  wchar_t revs[25];
  char ch;

  if (pNotation == 2)
    ch = 'b';
  else if (pNotation == 8)
    ch = 'o';
  else
    ch = 'x';

  revs[0] = '0';
  revs[1] = ch;

  while (pValue >= pNotation) {
    temp[i] = pValue % pNotation;
    pValue /= pNotation;
    i++;
  }
  temp[i] = pValue % pNotation;
  len = i + 1;
  for (i = 0; i < len; i++)
    if (pNotation != 16)
      temp[i] += '0';
    else
      temp[i] += temp[i] >= 10 ? 'A' - 10 : '0';

  // 반전시키는 코드가 들어가야함
  for (i = 0; i < len; i++)
    revs[2 + i] = temp[len - 1 - i];
  revs[2 + len] = '\0';

  return String(revs);
}