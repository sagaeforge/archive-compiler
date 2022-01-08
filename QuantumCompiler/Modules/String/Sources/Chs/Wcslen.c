
#include "Chs.h"

Length __Wcslen(const_wcs Value) {
  if (Value == NULL)
    return 0;

  Index i = 0;
  while (Value[i] != L'\0')
    i++;
  return i;
}