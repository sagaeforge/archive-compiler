
#include <Module/nString.h>

int
__ToLower(const int pCh)
{
  return __IsUpper(pCh) ? 'a' + (pCh - 'A') : pCh;
}
