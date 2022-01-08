
#ifndef __CHARSTRING__
#define __CHARSTRING__

#include "String.h"
Length __Chslen(const_chs Value);
Length __Wcslen(const_wcs Value);
wcs __WcsCreate(Length Length);
void __WcsChsSet(wcs Obj1, const_chs Obj2, Length Length);
void __WcsWcsSet(wcs Obj1, const_wcs Obj2, Length Length);

bool __IsUpper(int ch);
bool __IsLower(int ch);
bool __IsAlpha(int ch);
bool __IsDecimal(int ch);
bool __IsSpace(int ch);
bool __IsHex(int ch);
bool __IsOctal(int ch);
bool __IsBinary(int ch);
bool __IsControl(int ch);
int __ToUpper(int ch);
int __ToLower(int ch);

#endif