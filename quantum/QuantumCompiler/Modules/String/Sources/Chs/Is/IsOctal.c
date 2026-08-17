
#include <CString.h>
#include <Chs.h>

bool
__IsOctal(int pCh)
{
  return pCh >= '0' && pCh <= '8';
}

bool
IsOctal(int pCh)
{
  return __IsOctal(pCh);
}