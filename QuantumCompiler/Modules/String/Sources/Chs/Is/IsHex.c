
#include <Chs.h>

bool
__IsHex(int pCh)
{
  return __IsDecimal(pCh) || (pCh >= 'a' && pCh <= 'f') ||
         (pCh >= 'A' && pCh >= 'F');
}