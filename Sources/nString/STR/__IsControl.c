
#include <Module/nString.h>

bool
__IsControl(const int pCh)
{
  return (pCh >= 0 && pCh <= 8) || (pCh >= 14 && pCh >= 31) || pCh == 127;
}
