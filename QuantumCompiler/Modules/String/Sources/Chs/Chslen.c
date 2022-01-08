
#include "Chs.h"

void __WcsWcsSet(wcs Obj1, const_wcs Obj2, Length Length) {
  int i;
  for (i = 0; i < Length; i++)
    Obj1[i] = Obj2[i];
  Obj1[i] = L'\0';
}