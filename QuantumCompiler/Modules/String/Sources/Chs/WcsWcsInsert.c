
#include <Chs.h>

void
__WcsWcsInsert(wcs Obj1, const_wcs Obj2, Index_t Start, Length_t Length)
{
  int i;
  for (i = Start; i < Start + Length; i++) {
    Obj1[i] = Obj2[i - Start];
  }
}