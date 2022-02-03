
#include <Chs.h>

bool
__IsSpace(int pCh)
{
  return (pCh >= 9 && pCh <= 13) || pCh == 32;
}