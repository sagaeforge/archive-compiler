
#include <Private_String.h>

Index_t
String_IndexAt(String pSelf, String pValue, Index_t pStart)
{
  if (pSelf->Length <= pStart)
    return -1;

  int i;
  for (i = pStart; i < pSelf->Length - pValue->Length + 1; i++)
    if (pSelf->Value[i] == pValue->Value[0])
      if (_StringCompare(pSelf->Value, pValue, i))
        return i;
  return -1;
}