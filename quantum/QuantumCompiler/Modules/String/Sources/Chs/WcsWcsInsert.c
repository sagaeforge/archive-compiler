
#include <Chs.h>

void
__WcsWcsInsert(wcs pObj1, const_wcs pObj2, Index_t pStart, Length_t pLength)
{
  int i;
  for (i = pStart; i < pStart + pLength; i++) {
    pObj1[i] = pObj2[i - pStart];
  }
}