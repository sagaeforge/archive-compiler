
#ifndef __CHARSTRING__
#define __CHARSTRING__

#include "String.h"

Length_t
__StrLen(void* Obj, Length_t WordSize);
wcs
__WcsCreate(Length_t Length);
void
__StrSet(wcs Obj1, const void* Obj2, Length_t WordSize, Length_t Length);
void
__WcsWcsInsert(wcs Obj1, const_wcs Obj2, Length_t Start, Length_t Length);

bool
__IsUpper(int ch);
bool
__IsLower(int ch);
bool
__IsAlpha(int ch);
bool
__IsDecimal(int ch);
bool
__IsSpace(int ch);
bool
__IsHex(int ch);
bool
__IsOctal(int ch);
bool
__IsBinary(int ch);
bool
__IsControl(int ch);
int
__ToUpper(int ch);
int
__ToLower(int ch);

#endif