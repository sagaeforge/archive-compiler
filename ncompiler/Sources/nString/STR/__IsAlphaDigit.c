
#include <Module/nString.h>

bool
__IsAlphaDigit(const int pCh)
{
  return __IsAlpha(pCh) || __IsDigit(pCh);
}
