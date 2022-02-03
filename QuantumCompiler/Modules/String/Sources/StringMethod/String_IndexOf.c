
#include <Private_String.h>

Index_t
String_IndexOf(String pSelf, String pValue)
{
  int i;
  for (i = 0; i < pSelf->Length - pValue->Length + 1; i++)
    if (pSelf->Value[i] == pValue->Value[0])
      if (_StringCompare(pSelf->Value, pValue, i))
        return i;
  return -1;
}