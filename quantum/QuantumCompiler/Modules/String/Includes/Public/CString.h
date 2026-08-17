

#ifndef __PUBLIC_STRING_CHARSTRING__
#define __PUBLIC_STRING_CHARSTRING__

#include <Types/DataType.h>

// clang-format off

bool      IsUpper       (int pCh);
bool      IsLower       (int pCh);
bool      IsAlpha       (int pCh);
bool      IsDecimal     (int pCh);
bool      IsSpace       (int pCh);
bool      IsHex         (int pCh);
bool      IsOctal       (int pCh);
bool      IsBinary      (int pCh);
bool      IsControl     (int pCh);
int       ToUpper       (int pCh);
int       ToLower       (int pCh);

#endif