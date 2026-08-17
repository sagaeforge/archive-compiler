
#include <Module/nString.h>

Length_t
__WcsLen(const Wcs_t pValue)
{
  Length_t _i = 0;
  while (pValue[_i] != '\0')
    _i++;
  return _i;
}
