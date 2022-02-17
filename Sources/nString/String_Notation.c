
#include <stdlib.h>

#include <Module/nString.h>

nString_t*
String_Notation(const int64_t pValue, const int pNotation)
{
  if (pNotation != 2 && pNotation != 8 && pNotation != 16)
    return String_ToString_Decimal_Unsigned(pValue);

  int64_t _Value = pValue;

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

  while (_Value >= pNotation) {
    temp[i] = _Value % pNotation;
    _Value /= pNotation;
    i++;
  }
  temp[i] = _Value % pNotation;
  len = i + 1;
  for (i = 0; i < len; i++)
    if (pNotation != 16)
      temp[i] += '0';
    else
      temp[i] += temp[i] >= 10 ? 'A' - 10 : '0';

  for (i = 0; i < len; i++)
    revs[2 + i] = temp[len - 1 - i];
  revs[2 + len] = '\0';

  return nString(revs);
}
