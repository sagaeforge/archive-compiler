
#ifndef __PRIVATE_STRING_CHARSTRING__
#define __PRIVATE_STRING_CHARSTRING__

#include <String.h>

// clang-format off

Length_t  __StrLen        (void* pObj, Length_t pWordSize);
wcs       __WcsCreate     (Length_t pLength);
void      __StrSet        (wcs pObj1, const void* pObj2, Length_t pWordSize, Length_t pLength);
void      __WcsWcsInsert  (wcs pObj1, const_wcs pObj2, Length_t pStart, Length_t pLength);

bool      __IsUpper       (int pCh);
bool      __IsLower       (int pCh);
bool      __IsAlpha       (int pCh);
bool      __IsDecimal     (int pCh);
bool      __IsSpace       (int pCh);
bool      __IsHex         (int pCh);
bool      __IsOctal       (int pCh);
bool      __IsBinary      (int pCh);
bool      __IsControl     (int pCh);
int       __ToUpper       (int pCh);
int       __ToLower       (int pCh);

#endif