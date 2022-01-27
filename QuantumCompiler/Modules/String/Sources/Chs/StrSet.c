
#include <Chs.h>

static void
__WcsSet(wcs Obj1, wcs Obj2, Length_t Length)
{
  int i;
  for (i = 0; i < Length; i++)
    Obj1[i] = Obj2[i];
}

static void
__ChsSet(wcs Obj1, chs Obj2, Length_t Length)
{
  int i;
  for (i = 0; i < Length; i++)
    Obj1[i] = Obj2[i];
}

void
__StrSet(wcs Obj1, const void* Obj2, Length_t WordSize, Length_t Length)
{
  if (WordSize == 1)
    __ChsSet(Obj1, (chs)Obj2, Length);
  else
    __WcsSet(Obj1, (wcs)Obj2, Length);
}