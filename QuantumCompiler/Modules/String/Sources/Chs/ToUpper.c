
#include <Chs.h>

int
__ToUpper(int pCh)
{
  return __IsLower(pCh) ? 'A' + (pCh - 'a') : pCh;
}

int
ToUpper(int pCh)
{
  return __ToUpper(pCh);
}