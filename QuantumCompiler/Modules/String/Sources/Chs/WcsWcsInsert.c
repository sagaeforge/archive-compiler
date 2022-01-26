
#include "Chs.h"

void
__WcsWcsInsert(wcs Obj1, const_wcs Obj2, Index_t Start, Length_t Length)
{
  // TODO 오류 검사
  int i;
  for (i = Start; i < Start + Length; i++) {
    Obj1[i] = Obj2[i - Start];
  }
}