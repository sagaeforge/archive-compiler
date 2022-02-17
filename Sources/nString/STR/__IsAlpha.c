
#include <Module/nString.h>

bool
__IsAlpha(const int pCh)
{
  return (pCh >= 'A' && pCh <= 'Z') || (pCh >= 'a' && pCh <= 'z');
}
