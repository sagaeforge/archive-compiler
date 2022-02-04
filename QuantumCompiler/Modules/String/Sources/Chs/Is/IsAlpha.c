
#include <CString.h>
#include <Chs.h>

bool
__IsAlpha(int pCh)
{
  return (pCh >= 'A' && pCh <= 'Z') || (pCh >= 'a' && pCh <= 'z');
}

bool
IsAlpha(int pCh)
{
  return __IsAlpha(pCh);
}