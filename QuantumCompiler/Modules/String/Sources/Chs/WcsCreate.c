
#include "Chs.h"

Length
__Chslen(const_chs Value)
{
  if (Value == NULL)
    return 0;

  Index i = 0;
  while (Value[i] != '\0')
    i++;
  return i;
}