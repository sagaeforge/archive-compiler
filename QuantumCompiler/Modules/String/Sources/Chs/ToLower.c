
#include "Chs.h"

int __ToLower(int ch) { return __IsUpper(ch) ? 'a' + (ch - 'A') : ch; }