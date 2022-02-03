
#include <Private_String.h>

bool
String_Compare(String pSelf, String pValue)
{
  if (pSelf->Length != pValue->Length)
    return false;

  int i;
  for (i = 0; i < pSelf->Length; i++)
    if (pSelf->Value[i] != pValue->Value[i])
      return false;
  return true;
}
