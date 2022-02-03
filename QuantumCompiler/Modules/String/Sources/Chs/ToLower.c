
#include <Chs.h>

int
__ToLower(int pCh)
{
  return __IsUpper(pCh) ? 'a' + (pCh - 'A') : pCh;
}