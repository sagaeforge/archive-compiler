
#include <Module/nString.h>

int
__ToUpper(const int pCh)
{
  return __IsLower(pCh) ? 'A' + (pCh - 'a') : pCh;
}
