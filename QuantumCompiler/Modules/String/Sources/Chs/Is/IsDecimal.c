
#include <CString.h>
#include <Chs.h>

bool
__IsDecimal(int pCh)
{
  return pCh >= '0' && pCh <= '9';
}

bool
IsDecimal(int pCh)
{
  return __IsDecimal(pCh);
}