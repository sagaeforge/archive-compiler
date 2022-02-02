
#include <Private_String.h>

bool
_StringCompare(wcs Ary, String FindValue, Index_t Start)
{
  int i;
  for (i = Start; i < Start + FindValue->Length; i++)
    if (Ary[i] != FindValue->Value[i - Start])
      return false;
  return true;
}