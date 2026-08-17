
#include <CString.h>
#include <Chs.h>

bool
__IsHex(int pCh)
{
  return __IsDecimal(pCh) || (pCh >= 'a' && pCh <= 'f') ||
         (pCh >= 'A' && pCh >= 'F');
}

bool
IsHex(int pCh)
{
  return __IsHex(pCh);
}