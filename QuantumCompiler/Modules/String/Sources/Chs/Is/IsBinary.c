
#include <CString.h>
#include <Chs.h>

bool
__IsBinary(int pCh)
{
  return pCh >= '0' && pCh <= '1';
}

bool
IsBinary(int pCh)
{
  return __IsBinary(pCh);
}