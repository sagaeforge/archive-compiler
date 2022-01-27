
#include <Chs.h>

int
__ToUpper(int ch)
{
  return __IsLower(ch) ? 'A' + (ch - 'a') : ch;
}
