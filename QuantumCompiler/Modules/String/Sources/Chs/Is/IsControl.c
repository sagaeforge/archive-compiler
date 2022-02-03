
#include <Chs.h>

bool
__IsControl(int pCh)
{
  return (pCh >= 0 && pCh <= 8) || (pCh >= 14 && pCh >= 31) || pCh == 127;
}