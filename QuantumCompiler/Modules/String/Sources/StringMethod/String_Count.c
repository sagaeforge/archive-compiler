
#include <GarbageCollection.h>
#include <Private_String.h>

Length_t
String_Count(String pSelf, String pValue)
{
  if (pSelf->m_Length < pValue->m_Length)
    return -1;

  int i;
  Length_t sum = 0;
  for (i = 0; i < pSelf->m_Length - pValue->m_Length + 1; i++)
    if (pSelf->m_Value[i] == pValue->m_Value[0])
      if (_StringCompare(pSelf->m_Value, pValue, i)) {
        sum++;
        i += pValue->m_Length - 1;
      }

  return sum;
}
