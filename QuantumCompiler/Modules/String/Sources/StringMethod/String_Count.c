
#include <GarbageCollection.h>
#include <Private_String.h>

Length_t
String_Count(String pSelf, String pValue)
{
  if (pSelf->Length < pValue->Length)
    return -1;

  int i;
  Length_t sum = 0;
  for (i = 0; i < pSelf->Length - pValue->Length + 1; i++)
    if (pSelf->Value[i] == pValue->Value[0])
      if (_StringCompare(pSelf->Value, pValue, i)) {
        sum++;
        i += pValue->Length - 1;
      }

  return sum;
}
