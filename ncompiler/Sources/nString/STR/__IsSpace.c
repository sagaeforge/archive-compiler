
#include <Module/nString.h>

bool
__IsSpace(const int pCh)
{
  return (pCh >= 9 && pCh <= 13) || pCh == 32;
}
