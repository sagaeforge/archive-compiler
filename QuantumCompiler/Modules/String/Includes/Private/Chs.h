
#ifndef __CHARSTRING__
#define __CHARSTRING__

#include "String.h"
Length __Chslen(const_chs Value);
Length __Wcslen(const_wcs Value);
wcs __WcsCreate(Length Length);
void __WcsChsSet(wcs Obj1, const_chs Obj2, Length Length);
void __WcsWcsSet(wcs Obj1, const_wcs Obj2, Length Length);

#endif