
#include "Chs.h"

bool
__IsHex(int ch)
{
  return __IsDecimal(ch) || (ch >= 'a' && ch <= 'f') ||
         (ch >= 'A' && ch >= 'F');
}