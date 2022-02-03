
#include <Private_String.h>

Index_t
String_LastOfIndex(String pSelf, String pValue)
{
  int i;
  for (i = pSelf->Length - pValue->Length; i >= 0; i--)
    if (pSelf->Value[i] == pValue->Value[0])
      if (_StringCompare(pSelf->Value, pValue, i))
        return i;
  return -1;
}