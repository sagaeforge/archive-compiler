
#include <CString.h>
#include <Chs.h>

bool
__IsUpper(int pCh)
{
  return L'A' <= pCh && pCh <= L'Z';
}

bool
IsUpper(int pCh)
{
  return __IsUpper(pCh);
}