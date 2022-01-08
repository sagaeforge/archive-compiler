
#include "Chs.h"

void __WcsChsSet(wcs Obj1, const_chs Obj2, Length Length) {
  int i;
  for (i = 0; i < Length; i++)
    Obj1[i] = Obj2[i];
  Obj1[i] = L'\0';
}