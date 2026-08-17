
#include <Chs.h>

static void
__WcsSet(wcs pObj1, wcs pObj2, Length_t pLength)
{
  int i;
  for (i = 0; i < pLength; i++)
    pObj1[i] = pObj2[i];
}

static void
__ChsSet(wcs pObj1, chs pObj2, Length_t pLength)
{
  int i;
  for (i = 0; i < pLength; i++)
    pObj1[i] = pObj2[i];
}

void
__StrSet(wcs pObj1, const void* pObj2, Length_t pWordSize, Length_t pLength)
{
  if (pWordSize == 1)
    __ChsSet(pObj1, (chs)pObj2, pLength);
  else
    __WcsSet(pObj1, (wcs)pObj2, pLength);
}